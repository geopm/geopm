#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# This script is the launcher for platform power & frequency sweeps
# for system characterization

export GEOPM_SOURCE="${GEOPM_SOURCE?- Set path to GEOPM source code}"
export SCRIPT_DIR="${GEOPM_SOURCE}/integration/experiment/system_characterization"
source ${SCRIPT_DIR}/set_vars.sh
source ${SCRIPT_DIR}/utils.sh


# Run frequency sweeps under multiple platform-level power caps
START_TIME=${SECONDS}

for ((l="$BOARD_MIN_POWER"; l<="$BOARD_MAX_POWER"; l=l+"$BOARD_POWER_STEP")); do

        echo "sweep type is $SWEEP_TYPE "

        if [ "${SWEEP_TYPE}" == "CPU" ]; then
                launch_cpu_sweep 
        elif [ "${SWEEP_TYPE}" == "GPU" ]; then
                launch_gpu_sweep
        else
                echo "*** Incorrect SWEEP TYPE ***"
                exit 0
        fi
done

END_TIME=${SECONDS}

SECONDS_ELAPSED=$(( ${END_TIME} - ${START_TIME} ))
MINUTES_ELAPSED=$(( ${SECONDS_ELAPSED} / 60 ))
HOURS=$(( ${MINUTES_ELAPSED} / 60 ))
MINUTES=$(( ${MINUTES_ELAPSED} % 60 ))
SECONDS=$(( ${SECONDS_ELAPSED} % 60 ))

echo "INFO: Job took ${HOURS} hours, ${MINUTES} minutes, and ${SECONDS} seconds."
echo "INFO: Complete."
