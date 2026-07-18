#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# Exercise the #4044 + #4043 energy objective: with --efficiency and no
# --metric-regex, geopmopt minimizes total energy measured from a rollover-safe
# geopmsession energy trace.  The run must complete without a "non-positive
# power" abort and emit a valid control configuration.

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
         --efficiency=cpu \
         --output-file="${output_config}" \
         --trials=20 \
         -- ./check_geopmopt_dgemm_bench_run.sh

# The energy objective ran to completion and selected a CPU frequency setting.
# The energy-optimal frequency is hardware dependent, so assert only that a
# valid configuration was produced rather than pinning a specific frequency.
grep -q "CPU_FREQUENCY_MAX_CONTROL board 0" "${output_config}"
