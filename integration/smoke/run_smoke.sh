#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

source smoke_env.sh

APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg hpl_mkl hpl_netlib pennant"

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
    EXP_SUBDIR=monitor
    EXP_TYPE=monitor
    ARGS=""
    run_all
}

function run_all_power_sweep {
    EXP_SUBDIR=power_sweep
    EXP_TYPE=power_sweep
    ARGS="--min-power=190 --max-power=230"
    run_all
}

function run_all_freq_sweep {
    EXP_SUBDIR=frequency_sweep
    EXP_TYPE=frequency_sweep
    ARGS="--min-frequency=1.9e9 --max-frequency=2.0e9"
    run_all
}

function run_all_uncore_freq_sweep {
    EXP_SUBDIR=uncore_frequency_sweep
    EXP_TYPE=uncore_frequency_sweep
    ARGS="--min-frequency=1.9e9 --max-frequency=2.0e9 --min-uncore-frequency=2.1e9 --max-uncore-frequency=2.2e9"
    run_all
}

function run_all_power_balancer_energy {
    EXP_SUBDIR=energy_efficiency
    EXP_TYPE=power_balancer_energy
    ARGS="--min-power=220 --max-power=230"
    run_all
}

function run_all_barrier_freq_sweep {
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
    echo "Usage: $0 monitor|power_sweep|freq_sweep|power_balancer_energy|barrier_freq_sweep|uncore_freq_sweep"
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
elif [ "$name" == "uncore_freq_sweep" ]; then
    run_all_uncore_freq_sweep
else
    echo "Error: Unknown name: $name" 1>&2
    exit -1
fi
