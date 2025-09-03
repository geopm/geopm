#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

module load geopm-runtime
export GEOPM_SOURCE="${GEOPM_SOURCE?-Set GEOPM_SOURCE to full path of file with initialization parameters}"
export PYTHONPATH=${GEOPM_SOURCE}/:${GEOPM_SOURCE}/geopmpy/:${GEOPM_SOURCE}/geopmdpy/:${HOME}/.local/lib/python3.6/site-packages/:$PYTHONPATH

source ${GEOPM_SOURCE}/integration/experiment/system_characterization/set_vars.sh
source ${GEOPM_SOURCE}/integration/experiment/system_characterization/utils.sh

## Run frequency sweeps under multiple platform-level power caps
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
