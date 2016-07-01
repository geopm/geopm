#!/bin/bash
#  Copyright (c) 2015, 2016, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

test_name=`basename $0`
dir_name=`dirname $0`
run_test=true
err=0

if echo $test_name | grep -v '^MPI' > /dev/null; then
    # Check for crc32 intrinsic support before running LockingHashTable tests
    if echo $test_name | grep '^LockingHashTable' > /dev/null; then
        if  ! ./examples/geopm_platform_supported crc32; then
            echo "Warning: _mm_crc32_u64 intrisic not supported."
            run_test=false
        fi
    fi

    if [ "$run_test" == "true" ]; then
        # This is not an MPI test, run geopm_test
        $dir_name/../geopm_test --gtest_filter=$test_name >& $dir_name/$test_name.log
        err=$?
    fi
else
    # This is an MPI test set up environment discover execution
    # wrappwer and call geopm_mpi_test

    # Dynamic weak symbols are required for PMPI integration
    export LD_DYNAMIC_WEAK

    num_proc=16
    num_node=1

    # Determine the wrapper for running MPI jobs
    if [ "$MPIEXEC" ]; then
        # Use MPIEXEC environment variable if it is set
        mpiexec="$MPIEXEC"
    elif which srun >& /dev/null && \
        srun -N 4 true >& /dev/null; then
        # use slurm srun if in path
        mpiexec="srun -N 4"
        num_node=4
    elif which srun >& /dev/null && \
        srun -N 1 true >& /dev/null; then
        # use slurm srun if in path
        mpiexec="srun -N 1"
    elif which mpiexec >& /dev/null; then
        # use mpiexec if in path
        mpiexec="mpiexec"
    elif [ -x /usr/lib64/mpi/gcc/openmpi/bin/mpiexec ]; then
        # Stock OpenMPI version of mpiexec on SLES
        mpiexec=/usr/lib64/mpi/gcc/openmpi/bin/mpiexec
    elif [ -x /usr/lib64/openmpi/bin/mpiexec]; then
        # Stock OpenMPI version of mpiexec on RHEL
        mpiexec=/usr/lib64/openmpi/bin/mpiexec
    else
        echo "Error: MPIEXEC unset, and no alternative found." 2>&1
        exit -1
    fi

    # Enable GEOPM runtime variables for MPIProfile tests
    if echo $test_name | grep '^MPIProfile' > /dev/null && \
       echo $test_name | grep -v 'noctl' > /dev/null; then
       export GEOPM_POLICY=test/default_policy.json
       export GEOPM_PMPI_CTL=process
       export GEOPM_REPORT=geopm_report
       # Add a process for controller on each node
       num_proc=$(($num_proc + $num_node))
       num_cpu=$(lscpu | grep '^CPU(s):' | awk '{print $2}')
       if ! ./examples/geopm_platform_supported; then
          run_test=false
       fi
    fi

    if [ "$run_test" == "true" ]; then
        libtool --mode=execute $mpiexec -n $num_proc $dir_name/../geopm_mpi_test --gtest_filter=$test_name >& $dir_name/$test_name.log
        err=$?
    fi
fi

if [ "$run_test" != "true" ]; then
    echo "SKIP: $test_name"
fi


exit $err
