#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set err=0
. ../tutorial_env.sh

export PATH=$GEOPM_BINDIR:$PATH
export PYTHONPATH=$GEOPMPY_PKGDIR:$PYTHONPATH
export LD_LIBRARY_PATH=$GEOPM_LIBDIR:$LD_LIBRARY_PATH

# ensure both required plugins can be found
export GEOPM_TUTORIAL=..
export GEOPM_PLUGIN_PATH=$GEOPM_TUTORIAL/iogroup:$GEOPM_TUTORIAL/agent

echo "Redirecting output to example.stdout and example.stderr."


NUM_NODES=2
RANKS_PER_NODE=2
TOTAL_RANKS=$((${RANKS_PER_NODE} * ${NUM_NODES}))

if [ "$MPIEXEC" ]; then
    # Use MPIEXEC and set GEOPM environment variables to launch the job
    LD_DYNAMIC_WEAK=true \
    GEOPM_CTL=process \
    GEOPM_REPORT=agent_tutorial_report \
    GEOPM_TRACE=agent_tutorial_trace \
    GEOPM_AGENT=example \
    GEOPM_POLICY=example_policy.json \
    $MPIEXEC \
    geopmbench tutorial_6_config.json
    err=$?
elif [ "$GEOPM_LAUNCHER" = "srun" ]; then
    # Use GEOPM launcher wrapper script with SLURM's srun
    geopmlaunch srun \
                -N ${NUM_NODES} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-report=agent_tutorial_report \
                --geopm-trace=agent_tutorial_trace \
                --geopm-agent=example \
                --geopm-policy=example_policy.json \
                -- geopmbench agent_tutorial_config.json \
                1>example.stdout 2>example.stderr
    err=$?
elif [ "$GEOPM_LAUNCHER" = "aprun" ]; then
    # Use GEOPM launcher wrapper script with ALPS's aprun
    geopmlaunch aprun \
                -N ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-report=agent_tutorial_report \
                --geopm-trace=agent_tutorial_trace \
                --geopm-agent=example \
                --geopm-policy=example_policy.json \
                -- geopmbench agent_tutorial_config.json \
                1>example.stdout 2>example.stderr
    err=$?
elif [ "$GEOPM_LAUNCHER" = "impi" ]; then
    # Use GEOPM launcher wrapper script with Intel(R) MPI
    geopmlaunch impi \
                -ppn ${RANKS_PER_NODE} \
                -n ${TOTAL_RANKS} \
                --geopm-ctl=process \
                --geopm-report=agent_tutorial_report \
                --geopm-trace=agent_tutorial_trace \
                --geopm-agent=example \
                --geopm-policy=example_policy.json \
                -- geopmbench agent_tutorial_config.json \
                1>example.stdout 2>example.stderr
    err=$?
else
    echo "Error: agent_tutorial.sh: set GEOPM_LAUNCHER to 'srun' or 'aprun'." 2>&1
    echo "       If SLURM or ALPS are not available, set MPIEXEC to" 2>&1
    echo "       a command that will launch an MPI job on your system" 2>&1
    echo "       using 2 nodes and 10 processes." 2>&1
    err=1
fi

exit $err
