#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause


export SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/set_vars.sh

# Run frequency sweeps under multiple platform-level power caps
START_TIME=${SECONDS}

if [ "${SWEEP_TYPE}" == "CPU" ]; then
        launch_cpu_sweep 
elif [ "${SWEEP_TYPE}" == "GPU" ]; then
        launch_gpu_sweep
else
        echo "*** Incorrect SWEEP TYPE ***"
fi

END_TIME=${SECONDS}

SECONDS_ELAPSED=$(( ${END_TIME} - ${START_TIME} ))
MINUTES_ELAPSED=$(( ${SECONDS_ELAPSED} / 60 ))
HOURS=$(( ${MINUTES_ELAPSED} / 60 ))
MINUTES=$(( ${MINUTES_ELAPSED} % 60 ))
SECONDS=$(( ${SECONDS_ELAPSED} % 60 ))

echo "INFO: Job took ${HOURS} hours, ${MINUTES} minutes, and ${SECONDS} seconds."
echo "INFO: Complete."
