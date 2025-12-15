#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

output_config="optimal-frequency.config"
expected_config="expected.config"

cleanup() {
    rm -f "${output_config}" "${expected_config}"
}
trap cleanup EXIT

geopmopt --verbosity=2 \
         --cpu-frequency board \
         --defer-write \
         --minimize \
         --output-file "${output_config}" \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         -- ./check_geopmopt_dgemm_run.sh "${output_config}"

# For DGEMM expect maximum frequency minimizes time in dgemm
NUM_FREQ=$(geopmgrid --cpu-frequency board --coordinate-range)
geopmgrid --cpu-frequency board --coordinate $((NUM_FREQ - 1)) >"${expected_config}"
diff "${expected_config}" "${output_config}"
