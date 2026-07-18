#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# Exercise the #4044 default objective: with no --metric-regex, geopmopt
# minimizes the wall-clock runtime of the launch command directly.  dgemm is
# compute bound, so the maximum allowed frequency should minimize runtime.

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
         --cpu-frequency=board \
         --cpu-frequency-max=${MAX_FREQ} \
         --minimize \
         --output-file="${output_config}" \
         --trials=20 \
         -- ./check_geopmopt_dgemm_bench_run.sh

# For compute-bound DGEMM the maximum allowed frequency minimizes runtime.
grep -q ${MAX_FREQ} "${output_config}"
