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

# Run on 2 nodes
# with 8 total application MPI ranks
# load geopm runtime with LD_PRELOAD
# launch geopm controller as an MPI process
# create a report file
# create trace files

NUM_NODES=2
RANKS_PER_NODE=4
TOTAL_RANKS=$((${RANKS_PER_NODE} * ${NUM_NODES}))

if [ "$MPIEXEC" ]; then
    export GEOPM_PROGRAM_FILTER=tutorial_1
    # Use MPIEXEC to launch the Controller process on all nodes
    GEOPM_REPORT=tutorial_1_report \
    GEOPM_TRACE=tutorial_1_trace \
    GEOPM_NUM_PROC=${RANKS_PER_NODE} \
    $MPIEXEC -n ${NUM_NODES} -ppn 1 geopmctl &

    # Use MPIEXEC and set LD_PRELOAD to launch the job
    LD_PRELOAD=$GEOPM_LIB/libgeopm.so \
    $MPIEXEC -n ${TOTAL_RANKS} -ppn ${RANKS_PER_NODE} \
    ./tutorial_1
    err=$?
elif [ "$GEOPM_LAUNCHER" = "srun" ]; then
    # Use GEOPM launcher wrapper script with SLURM's srun
    geopmlaunch srun \
                -N ${NUM_NODES} \
                -n ${TOTAL_RANKS} \
                --geopm-preload \
                --geopm-ctl=application \
                --geopm-report=tutorial_1_report \
                --geopm-trace=tutorial_1_trace \
                --geopm-program-filter=tutorial_1 \
                -- ./tutorial_1
    err=$?
elif [ "$GEOPM_LAUNCHER" = "aprun" ]; then
    # Use GEOPM launcher wrapper script with ALPS's aprun
    geopmlaunch aprun \
                -N ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-preload \
                --geopm-ctl=application \
                --geopm-report=tutorial_1_report \
                --geopm-trace=tutorial_1_trace \
                --geopm-program-filter=tutorial_1 \
                -- ./tutorial_1
    err=$?
elif [ "$GEOPM_LAUNCHER" = "impi" ]; then
    # Use GEOPM launcher wrapper script with Intel(R) MPI
    geopmlaunch impi \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-preload \
                --geopm-ctl=application \
                --geopm-report=tutorial_1_report \
                --geopm-trace=tutorial_1_trace \
                --geopm-program-filter=tutorial_1 \
                -- ./tutorial_1
    err=$?
elif [ "$GEOPM_LAUNCHER" = "ompi" ]; then
    # Use GEOPM launcher wrapper script with Open MPI
    geopmlaunch ompi \
                --npernode ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --hostfile tutorial_hosts \
                --geopm-preload \
                --geopm-ctl=application \
                --geopm-report=tutorial_1_report \
                --geopm-trace=tutorial_1_trace \
                --geopm-program-filter=tutorial_1 \
                -- ./tutorial_1
    err=$?
elif [ "$GEOPM_LAUNCHER" = "pals" ]; then
    # Use GEOPM launcher wrapper script with PALS
    geopmlaunch pals \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-preload \
                --geopm-ctl=application \
                --geopm-report=tutorial_1_report \
                --geopm-trace=tutorial_1_trace \
                --geopm-program-filter=tutorial_1 \
                -- ./tutorial_1
    err=$?
else
    echo "Error: tutorial_1.sh: set GEOPM_LAUNCHER to 'srun' or 'aprun'." 2>&1
    echo "       If SLURM or ALPS are not available, set MPIEXEC to" 2>&1
    echo "       a command that will launch an MPI job on your system" 2>&1
    echo "       using 2 nodes and 10 processes." 2>&1
    err=1
fi

exit $err
