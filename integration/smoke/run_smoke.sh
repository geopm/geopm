#!/bin/bash
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

source geopm_env.sh
EXP_DIR=$GEOPM_SRC/integration/experiment

function run_all {
    for APP in $APPLICATIONS; do
        SCRIPT=${EXP_DIR}/${EXP_SUBDIR}/run_${EXP_TYPE}_${APP}.py
        OUTDIR=${SLURM_JOB_ID}_${APP}_${EXP_TYPE}
        if [ -f $SCRIPT ]; then
            python3 ${SCRIPT} --output-dir=${OUTDIR} --node-count=${SLURM_NNODES} ${ARGS}
        else
            echo "Script missing: $SCRIPT"
        fi
    done
}

function run_all_monitor {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg"
    EXP_SUBDIR=monitor
    EXP_TYPE=monitor
    ARGS=""
    run_all
}

function run_all_power_sweep {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg"
    EXP_SUBDIR=power_sweep
    EXP_TYPE=power_sweep
    ARGS="--min-power=190 --max-power=230"
    run_all
}

function run_all_freq_sweep {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg"
    EXP_SUBDIR=frequency_sweep
    EXP_TYPE=frequency_sweep
    ARGS="--min-frequency=1.9e9 --max-frequency=2.0e9"
    run_all
}

function run_all_power_balancer_energy {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg"
    EXP_SUBDIR=energy_efficiency
    EXP_TYPE=power_balancer_energy
    ARGS="--min-power=220 --max-power=230"
    run_all
}

function run_all_barrier_freq_sweep {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg"
    EXP_SUBDIR=energy_efficiency
    EXP_TYPE=barrier_frequency_sweep
    ARGS="--min-core-frequency=1.9e9 --max-core-frequency=2.0e9"
    run_all
}

if [ ! "$SLURM_JOB_ID" ] || [ ! "$SLURM_NNODES" ]; then
    echo "Error: This script must be run within a SLURM allocation"
    exit -1
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 monitor|power_sweep|freq_sweep|power_balancer_energy|barrier_freq_sweep"
    exit -1
fi

name=$1

if [ "$name" == "monitor" ]; then
    run_all_monitor
elif [ "$name" == "power_sweep" ]; then
    run_all_power_sweep
elif [ "$name" == "freq_sweep" ]; then
    run_all_freq_sweep
elif [ "$name" == "power_balancer_energy" ]; then
    run_all_power_balancer_energy
elif [ "$name" == "barrier_freq_sweep" ]; then
    run_all_barrier_freq_sweep
else
    echo "Error: Unknown name: $name" 1>&2
    exit -1
fi
