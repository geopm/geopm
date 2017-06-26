#!/bin/bash
#
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

set err=0
source tutorial_env.sh

export PATH=$GEOPM_BINDIR:$PATH
export PYTHONPATH=$GEOPMPY_PKGDIR:$PYTHONPATH
export LD_LIBRARY_PATH=$GEOPM_LIBDIR:$LD_LIBRARY_PATH

# Run on 2 nodes
# with 8 MPI ranks
# launch geopm controller as an MPI process
# create a report file
# create trace files
if [ "$GEOPM_RM" == "SLURM" ]; then
    # Use GEOPM launcher wrapper script with SLURM's srun
    geopmsrun  -N 2 \
               -n 8 \
               --geopm-ctl=process \
               --geopm-report=tutorial_3_governed_report \
               --geopm-trace=tutorial_3_governed_trace \
               --geopm-policy=tutorial_governed_policy.json \
               -- ./tutorial_3 \
    && \
    geopmsrun  -N 2 \
               -n 8 \
               --geopm-ctl=process \
               --geopm-report=tutorial_3_balanced_report \
               --geopm-trace=tutorial_3_balanced_trace \
               --geopm-policy=tutorial_balanced_policy.json \
               -- ./tutorial_3
    err=$?

elif [ "$GEOPM_RM" == "ALPS" ]; then
    # Use GEOPM launcher wrapper script with ALPS's aprun
    geopmaprun -N 4 \
               -n 8 \
               --geopm-ctl=process \
               --geopm-report=tutorial_3_governed_report \
               --geopm-trace=tutorial_3_governed_trace \
               --geopm-policy=tutorial_governed_policy.json \
               -- ./tutorial_3 \
    && \
    geopmaprun -N 4 \
               -n 8 \
               --geopm-ctl=process \
               --geopm-report=tutorial_3_balanced_report \
               --geopm-trace=tutorial_3_balanced_trace \
               --geopm-policy=tutorial_balanced_policy.json \
               -- ./tutorial_3
    err=$?
elif [ $MPIEXEC ]; then
    LD_DYNAMIC_WEAK=true \
    GEOPM_PMPI_CTL=process \
    GEOPM_REPORT=tutorial_3_governed_report \
    GEOPM_TRACE=tutorial_3_governed_trace \
    GEOPM_POLICY=tutorial_governed_policy.json \
    $MPIEXEC ./tutorial_3
    && \
    LD_DYNAMIC_WEAK=true \
    GEOPM_PMPI_CTL=process \
    GEOPM_REPORT=tutorial_3_balanced_report \
    GEOPM_TRACE=tutorial_3_balanced_trace \
    GEOPM_POLICY=tutorial_balanced_policy.json \
    $MPIEXEC ./tutorial_3
    err=$?
else
    echo "Error: tutorial_3.sh: set GEOPM_RM to 'SLURM' or 'ALPS'." 2>&1
    echo "       If SLURM or ALPS are not available, set MPIEXEC to" 2>&1
    echo "       a command that will launch an MPI job on your system" 2>&1
    echo "       using 2 nodes and 10 processes." 2>&1
    err=1
fi

exit $err
