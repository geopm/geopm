#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

output_config="optimal-power.config"

cleanup() {
    rm -f "${output_config}"
}
trap cleanup EXIT

MAX_POWER=$(geopmread CPU_POWER_LIMIT_DEFAULT board 0)
MAX_POWER=$((MAX_POWER - 50))

geopmopt --verbosity=2 \
         --cpu-power=board \
         --cpu-power-max=${MAX_POWER} \
         --defer-write \
         --minimize \
         --output-file="${output_config}" \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=20 \
         -- ./check_geopmopt_dgemm_power_ctl_run.sh "${output_config}"

# For DGEMM expect maximum power minimizes time in dgemm
grep -q ${MAX_POWER} "${output_config}"
