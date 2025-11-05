#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# This script is the launcher for platform power & frequency sweeps
# for node characterization


export SWEEP_TYPE="${SWEEP_TYPE:? Set SWEEP_TYPE to either CPU or GPU}"
export GEOPM_SOURCE="${GEOPM_SOURCE:? Set GEOPM_SOURCE to path to GEOPM source code}"
export EMPTY_SWEEP_OUTPUT_DIR="${EMPTY_SWEEP_OUTPUT_DIR:? Set EMPTY_SWEEP_OUTPUT_DIR to an empty directory accessible from the compute node being characterized}"

export SCRIPT_DIR="${GEOPM_SOURCE}/integration/experiment/system_characterization"
source ${SCRIPT_DIR}/set_vars.sh ${SWEEP_TYPE} # this script checks for validity of ${1}
source ${SCRIPT_DIR}/utils.sh


# Run frequency sweeps under multiple platform-level power caps
START_TIME=${SECONDS}

for ((REQ_PLATFORM_CAP="${BOARD_MIN_POWER}"; REQ_PLATFORM_CAP<="${BOARD_MAX_POWER}"; REQ_PLATFORM_CAP=REQ_PLATFORM_CAP+"${BOARD_POWER_STEP}")); do

    echo "Sweep type is ${SWEEP_TYPE} "

    if [[ "${SWEEP_TYPE}" == "CPU" ]]; then
        launch_cpu_sweep
    elif [[ "${SWEEP_TYPE}" == "GPU" ]]; then
        launch_gpu_sweep
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
