#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex
MAX_POWER=$(geopmread CPU_POWER_LIMIT_DEFAULT board 0)
MAX_POWER=$((MAX_POWER - 50))
geopmopt --verbosity=2 \
         --cpu-power=board \
         --cpu-power-max=${MAX_POWER} \
         --defer-write \
         --minimize \
         --output-file=optimal-power.config \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=20 \
         -- ./check_geopmopt_dgemm_ctl_run.sh optimal-power.config

# For DGEMM expect maximum power minimizes time in dgemm
grep ${MAX_POWER} optimal-power.config
