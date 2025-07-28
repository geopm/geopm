#!/usr/bin/env bash

# Run application with GEOPM
# Create a report

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <config_file>"
    exit 1
fi

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
CONFIG_FILE=$1
REGION=intensity_32
CORE_COUNT=$(geopmread -d | grep "core" | awk '{print $2}')
RANK_COUNT=$((CORE_COUNT-4))
export OMP_NUM_THREADS=1
geopmlaunch pals \
            -n ${RANK_COUNT} -ppn ${RANK_COUNT} \
            --geopm-affinity-enable \
            --geopm-ctl=application \
            --geopm-profile=aib \
            --geopm-report=geopm.report \
            --geopm-program-filter=bench_avx512 \
            --geopm-init-control=${CONFIG_FILE} \
            --geopm-hyperthreads-disable \
            -- bench_avx512 -i 10
# Call python script to derive energy for REGION
python3 ${SCRIPT_DIR}/derive_energy.py geopm.report ${REGION}
