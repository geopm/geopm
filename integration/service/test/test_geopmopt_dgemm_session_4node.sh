#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

output_config="optimal-frequency.config"

cleanup() {
    rm -f "${output_config}"
}
trap cleanup EXIT

STICKER_FREQ=$(geopmread CPU_FREQUENCY_STICKER board 0)
MAX_FREQ=$((STICKER_FREQ - 300000000))

geopmopt --verbosity=2 \
         --sweep cpu-freq@board=:${MAX_FREQ} \
         --defer-write \
         --minimize \
         --output-file="${output_config}" \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=20 \
         -- ./check_geopmopt_dgemm_session_4node_run.sh "${output_config}"

# For DGEMM expect maximum frequency minimizes time in dgemm
grep -q ${MAX_FREQ} "${output_config}"
