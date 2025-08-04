#!/usr/bin/env bash

# Run application with GEOPM
# Create a report

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <config_file>"
    exit 1
fi

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
CONFIG_FILE=$1
REGION=stream
CORE_COUNT=$(geopmread -d | grep "core" | awk '{print $2}')
RANK_COUNT=2
RESERVED_CORES=${RESERVED_CORES:-4} # Default to 4 reserved cores if not set
export OMP_NUM_THREADS=$(((CORE_COUNT-RESERVED_CORES)/RANK_COUNT))
echo '{"loop-count": 1,"region": ["stream"],"big-o": [1.0]}' > geopmbench.conf
geopmlaunch pals \
            -n ${RANK_COUNT} -ppn ${RANK_COUNT} \
            --geopm-affinity-enable \
            --geopm-ctl=application \
            --geopm-profile=stream \
            --geopm-report=geopm.report \
            --geopm-program-filter=geopmbench \
            --geopm-init-control=${CONFIG_FILE} \
            --geopm-hyperthreads-disable \
            -- geopmbench geopmbench.conf
# Call python script to derive energy for REGION
python3 ${SCRIPT_DIR}/derive_energy.py geopm.report ${REGION}
