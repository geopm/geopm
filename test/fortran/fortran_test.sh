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

test_name=`basename $0`
dir_name=`dirname $0`

# Determine the wrapper for running MPI jobs
if [ "$MPIEXEC" ]; then
    # Use MPIEXEC environment variable if it is set
    mpiexec="$MPIEXEC"
elif command -v srun >& /dev/null && \
    srun -N 2 true >& /dev/null; then
    # use slurm srun if in path
    mpiexec="srun -N 2"
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

LD_PRELOAD=$dir_name/../../../.libs/libgeopm.so $mpiexec -n 2 $dir_name/../$test_name >& $dir_name/$test_name.log
err=$?

if [ $err -eq 0 ]; then
    ret=`grep "No Errors" $dir_name/$test_name.log  | wc -l`
    if [ $ret -gt 0 ]; then
        err=0;
    else
        err=-1
    fi
fi
exit $err
