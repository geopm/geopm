#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
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

# Run on 4 nodes
# with 8 total application MPI ranks
# launch geopm controller as an MPI process
# create a report file
# create trace files
HOSTNAME=$(hostname)

NUM_NODES=4
RANKS_PER_NODE=4
TOTAL_RANKS=$((${RANKS_PER_NODE} * ${NUM_NODES}))

# Determine the power policy for this system
MIN_POWER=$(geopmread CPU_POWER_MIN_AVAIL board 0)
MAX_POWER=$(geopmread CPU_POWER_MAX_AVAIL board 0)
AVG_POWER="$(( ( ${MAX_POWER} + ${MIN_POWER} ) / 2 ))"
geopmagent -a power_governor -p ${AVG_POWER} > tutorial_power_policy.json

if [ "$MPIEXEC_CTL" -a "$MPIEXEC_APP" ]; then
    export GEOPM_PROGRAM_FILTER=tutorial_4
    # Use MPIEXEC_CTL to launch the Controller process on all nodes
    GEOPM_AGENT="power_governor" \
    GEOPM_POLICY=tutorial_power_policy.json \
    GEOPM_REPORT=tutorial_4_governed_report_${HOSTNAME} \
    GEOPM_TRACE=tutorial_4_governed_trace \
    GEOPM_NUM_PROC=${RANKS_PER_NODE} \
    $MPIEXEC_CTL geopmctl &

    # Use MPIEXEC_APP to launch the job
    # Note: LD_PRELOAD is not needed as this app links against libgeopm
    $MPIEXEC_APP ./tutorial_4
    err=$?

    sleep 10 # Give the service a moment to release the CONTROL_LOCK

    if [ $err -eq 0 ]; then
        # Use MPIEXEC_CTL to launch the Controller process on all nodes
        GEOPM_AGENT="power_balancer" \
        GEOPM_POLICY=tutorial_power_policy.json \
        GEOPM_REPORT=tutorial_4_balanced_report_${HOSTNAME} \
        GEOPM_TRACE=tutorial_4_balanced_trace \
        GEOPM_NUM_PROC=${RANKS_PER_NODE} \
        $MPIEXEC_CTL geopmctl &

        # Use MPIEXEC_APP to launch the job
        # Note: LD_PRELOAD is not needed as this app links against libgeopm
        $MPIEXEC_APP ./tutorial_4
        err=$?
    fi
elif [ "$GEOPM_LAUNCHER" = "srun" ]; then
    # Use GEOPM launcher wrapper script with SLURM's srun
    geopmlaunch srun \
                -N ${NUM_NODES} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4 \
    && \
    geopmlaunch srun \
                -N ${NUM_NODES} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "aprun" ]; then
    # Use GEOPM launcher wrapper script with ALPS's aprun
    geopmlaunch aprun \
                -N ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4 \
    && \
    geopmlaunch aprun \
                -N ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "impi" ]; then
    # Use GEOPM launcher wrapper script with Intel(R) MPI
    geopmlaunch impi \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4 \
    && \
    geopmlaunch impi \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "ompi" ]; then
    # Use GEOPM launcher wrapper script with Open MPI
    geopmlaunch ompi \
                --npernode ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --hostfile tutorial_hosts \
                --geopm-ctl=application \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4 \
    && \
    geopmlaunch ompi \
                --npernode ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --hostfile tutorial_hosts \
                --geopm-ctl=application \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4
    err=$?
elif [ "$GEOPM_LAUNCHER" = "pals" ]; then
    # Use GEOPM launcher wrapper script with PALS
    geopmlaunch pals \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_governor \
                --geopm-report=tutorial_4_governed_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_governed_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4 \
    && \
    geopmlaunch pals \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=application \
                --geopm-agent=power_balancer \
                --geopm-report=tutorial_4_balanced_report_${HOSTNAME} \
                --geopm-trace=tutorial_4_balanced_trace \
                --geopm-policy=tutorial_power_policy.json \
                --geopm-program-filter=tutorial_4 \
                --geopm-affinity-enable \
                -- ./tutorial_4
    err=$?
else
    echo "Error: $0: Set either GEOPM_LAUNCHER or MPIEXEC_CTL/MPIEXEC_APP in your environment." 2>&1
    echo "       If no resource manager is available, set the MPIEXEC_* variables as follows:" 2>&1
    echo "       MPIEXEC_CTL: Run on 2 nodes, 1 process per node (2 total processes)" 2>&1
    echo "       MPIEXEC_APP: Run on 2 nodes, 4 processes per node (8 total processes)" 2>&1
    echo "" 2>&1
    echo "       E.g.:" 2>&1
    echo "       MPIEXEC_CTL=\"mpiexec -n 2 -ppn 1\" MPIEXEC_APP=\"mpiexec -n 8 -ppn 4\" $0" 2>&1
    err=1
fi

exit $err
