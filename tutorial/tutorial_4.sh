#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set err=0
. tutorial_env.sh

export PATH=$GEOPM_BIN:$PATH
export PYTHONPATH=$GEOPMPY_PKGDIR:$PYTHONPATH
export LD_LIBRARY_PATH=$GEOPM_LIB:$LD_LIBRARY_PATH

if [ ! $IMBALANCER_CONFIG ]; then
    export IMBALANCER_CONFIG=tutorial_4_imbalance.conf
fi
# Create configuration file for Imbalancer if it doesn't exist.
if [ ! -e $IMBALANCER_CONFIG ]; then
    echo $(hostname) 0.1 > $IMBALANCER_CONFIG
fi

# Run on 2 nodes
# with 8 total application MPI ranks
# launch geopm controller as an MPI process
# create a report file
# create trace files

NUM_NODES=2
RANKS_PER_NODE=4
TOTAL_RANKS=$((${RANKS_PER_NODE} * ${NUM_NODES}))
HOSTNAME=$(hostname)

if [ "$MPIEXEC" ]; then
    GEOPM_AGENT="power_governor" \
    LD_DYNAMIC_WEAK=true \
    GEOPM_CTL=process \
    GEOPM_REPORT=tutorial_4_governed_report_${HOSTNAME} \
    GEOPM_TRACE=tutorial_4_governed_trace \
    GEOPM_POLICY=tutorial_power_policy.json \
    $MPIEXEC ./tutorial_4 \
    && \
    GEOPM_AGENT="power_balancer" \
    LD_DYNAMIC_WEAK=true \
    GEOPM_PMPI_CTL=process \
    GEOPM_REPORT=tutorial_4_balanced_report_${HOSTNAME} \
    GEOPM_TRACE=tutorial_4_balanced_trace \
    GEOPM_POLICY=tutorial_power_policy.json \
    $MPIEXEC ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "srun" ]; then
    # Use GEOPM launcher wrapper script with SLURM's srun
    geopmlaunch srun \
                -N ${NUM_NODES} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4 \
    && \
    geopmlaunch srun \
                -N ${NUM_NODES} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "aprun" ]; then
    # Use GEOPM launcher wrapper script with ALPS's aprun
    geopmlaunch aprun \
                -N ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4 \
    && \
    geopmlaunch aprun \
                -N ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "impi" ]; then
    # Use GEOPM launcher wrapper script with Intel(R) MPI
    geopmlaunch impi \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4 \
    && \
    geopmlaunch impi \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "ompi" ]; then
    # Use GEOPM launcher wrapper script with Open MPI
    geopmlaunch ompi \
                --npernode ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --hostfile tutorial_hosts \
                --geopm-ctl=process \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4 \
    && \
    geopmlaunch ompi \
                --npernode ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --hostfile tutorial_hosts \
                --geopm-ctl=process \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                -- ./tutorial_4
    err=$?
else
    echo "Error: tutorial_4.sh: set GEOPM_LAUNCHER to 'srun' or 'aprun'." 2>&1
    echo "       If SLURM or ALPS are not available, set MPIEXEC to" 2>&1
    echo "       a command that will launch an MPI job on your system" 2>&1
    echo "       using 2 nodes and 10 processes." 2>&1
    err=1
fi

exit $err
