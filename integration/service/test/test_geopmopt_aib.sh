#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

AIB_INTENSITY=${AIB_INTENSITY:-2}
output_config="optimal-frequency-aib-${AIB_INTENSITY}.config"

cleanup() {
    rm -f "${output_config}"
}
trap cleanup EXIT

geopmopt --cpu-frequency=board \
         --cpu-uncore-frequency=board \
         --verbosity=2 \
         --defer-write \
         --minimize \
         --output-file="${output_config}" \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=40 \
         -- ./check_geopmopt_aib_run.sh "${output_config}"
