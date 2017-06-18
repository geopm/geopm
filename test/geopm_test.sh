#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

unset GEOPM_PMPI_CTL
unset GEOPM_POLICY
unset GEOPM_REPORT
unset GEOPM_TRACE

test_name=`basename $0`
dir_name=`dirname $0`
run_test=true
xml_dir=$dir_name
base_dir=$dir_name/../..

export LD_LIBRARY_PATH=$base_dir/.libs:$base_dir/openmp/lib:$LD_LIBRARY_PATH

if [[ $GTEST_XML_DIR ]]; then
    xml_dir=$GTEST_XML_DIR
fi
err=0

if [[ ! $test_name =~ ^MPI ]]; then
    # Check for crc32 intrinsic support before running ProfileTable tests
    if [[ $test_name =~ ^ProfileTable ]]; then
        if  ! ./examples/geopm_platform_supported crc32; then
            echo "Warning: _mm_crc32_u64 intrisic not supported."
            run_test=false
        fi
    fi

    # Skipped on Mac because the implementation is lax about invalid shmem construction
    if [[ $test_name == SharedMemoryTest.invalid_construction ]]; then
        if  [[ $(uname) == Darwin ]]; then
            run_test=false
        fi
    fi

    if [ "$run_test" == "true" ]; then
        # This is not an MPI test, run geopm_test
        $dir_name/../.libs/geopm_test \
            --gtest_filter=$test_name --gtest_output=xml:$xml_dir/$test_name.xml >& $dir_name/$test_name.log
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
        if echo $MPIEXEC | grep -q ^srun; then
            num_node=$(getopt -qu N: $MPIEXEC | awk '{print $2}')
        fi
    elif command -v srun >& /dev/null && \
        srun -N 4 true >& /dev/null; then
        # use slurm srun if in path
        mpiexec="srun -N 4"
        num_node=4
    elif command -v srun >& /dev/null && \
        srun -N 1 true >& /dev/null; then
        # use slurm srun if in path
        mpiexec="srun -N 1"
    elif command -v mpiexec >& /dev/null; then
        # use mpiexec if in path
        mpiexec="mpiexec"
    elif [ -x /usr/lib64/mpi/gcc/openmpi/bin/mpiexec ]; then
        # Stock OpenMPI version of mpiexec on SLES
        mpiexec=/usr/lib64/mpi/gcc/openmpi/bin/mpiexec
    elif [ -x /usr/lib64/openmpi/bin/mpiexec ]; then
        # Stock OpenMPI version of mpiexec on RHEL
        mpiexec=/usr/lib64/openmpi/bin/mpiexec
    else
        echo "Error: MPIEXEC unset, and no alternative found." 2>&1
        exit -1
    fi
    if [[ $test_name =~ ^MPITreeCommunicator ]] && $mpiexec --version 2>&1 | grep OpenRTE > /dev/null; then
        # If using OpenMPI, check that version is higher than 1.8.8 so
        # that MPITreeCommunicator tests don't hang due to issue here:
        # http://stackoverflow.com/questions/18737545/mpi-with-c-passive-rma-synchronization
        major=$(mpiexec --version 2>&1 | grep OpenRTE | awk '{print $3}' | awk -F\. '{print $1}')
        minor=$(mpiexec --version 2>&1 | grep OpenRTE | awk '{print $3}' | awk -F\. '{print $2}')
        hotfix=$(mpiexec --version 2>&1 | grep OpenRTE | awk '{print $3}' | awk -F\. '{print $3}')
        padded=$(printf %.3d%.3d%.3d $major $minor $hotfix)
        if [ "$padded" -lt "001008008" ]; then
           run_test=false
        fi
    fi

    # Enable GEOPM runtime variables for MPIProfile tests
    if [[ $test_name =~ ^MPIProfile ||
          $test_name =~ ^MPIController ]] &&
        [[ ! $test_name =~ noctl ]]; then
       export GEOPM_PMPI_CTL=process
       export GEOPM_REPORT=geopm_report

       if [[ $test_name =~ Death ]]; then
           export GEOPM_DEATH_TESTING=1
           if [[ "$mpiexec" =~ ^srun ]]; then
               mpiexec="srun -N 1"
           fi
           num_proc=2
       else
           # Add a process for controller on each node
           num_proc=$(($num_proc + $num_node))
       fi

       if ! ./examples/geopm_platform_supported; then
          run_test=false
       fi
    fi

    if [[ $test_name =~ ^MPIInterface ]]; then
       if [[ "$mpiexec" =~ ^srun ]]; then
           mpiexec="srun -N 1"
       fi
       num_proc=2
    fi

    if [ "$run_test" == "true" ]; then
        exec_name=geopm_mpi_test
        if [[ $test_name =~ ^MPIInterface ]]; then
            exec_name=geopm_mpi_test_api
        fi
        $mpiexec -n $num_proc \
            $dir_name/../.libs/$exec_name --gtest_filter=$test_name \
            --gtest_output=xml:$xml_dir/$test_name.xml>& $dir_name/$test_name.log
        err=$?

        # SLURM does not always handle death testing properly. Sometimes
        # killing an MPI rank will cause srun to return non-zero status,
        # but not all the time.  When death testing and we get a
        # non-zero status, check the test log to verify the status.
        # If the log does not contain the failed message, then it is a
        # false failure.
        if [[ $test_name =~ Death ]] && [ $err -eq 1 ]; then
            if (! grep -Fq "[  FAILED  ]" $dir_name/$test_name.log) &&
               (grep -Fq "[  PASSED  ] 1 test." $dir_name/$test_name.log); then
                echo "Overriding SLURM's status based on successful test log."
                err=0
            fi
        fi
    fi
fi

if [ "$run_test" != "true" ]; then
    echo "SKIP: $test_name"
else
    # Parse output log to see if the test actually ran.
    if (grep -Fq "[==========] Running 0 tests from 0 test cases." $dir_name/$test_name.log); then
        echo "ERROR: Test $test_name does not exist!"
        err=1
    fi
fi


exit $err
