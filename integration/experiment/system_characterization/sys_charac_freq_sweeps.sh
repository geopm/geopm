#!/bin/bash


export GEOPM_SOURCE="${GEOPM_SOURCE?-Set GEOPM_SOURCE to full path of file with initialization parameters}"

source ${GEOPM_SOURCE}/integration/experiment/system_characterization/set_env.sh
source ${GEOPM_SOURCE}/integration/experiment/system_characterization/set_vars.sh
source ${GEOPM_SOURCE}/integration/experiment/system_characterization/sys_charac_utils.sh

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
