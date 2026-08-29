#!/usr/bin/env bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Run application with GEOPM
# Create a report

set -x
set -e

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <config_file> <intensity>"
    exit 1
fi

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
CONFIG_FILE=$1
INTENSITY=$2
REGION=intensity_${INTENSITY}
CORE_COUNT=$(geopmread -d | grep "core" | awk '{print $2}')
RANK_COUNT=$((CORE_COUNT-4))


if [[ ${INTENSITY} -gt 16 ]]; then
    ITER_CNT=10
else
    ITER_CNT=20
fi

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
            -- bench_avx512 -i ${ITER_CNT} -b ${INTENSITY}
# Call python script to derive energy for REGION
python3 ${SCRIPT_DIR}/derive_energy.py geopm.report ${REGION}
