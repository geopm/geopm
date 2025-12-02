#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex
STICKER_FREQ=$(geopmread CPU_FREQUENCY_STICKER board 0)
MAX_FREQ=$((STICKER_FREQ - 300000000))
geopmopt --verbosity=2 \
         --cpu-frequency board \
         --cpu-frequency-max=${MAX_FREQ} \
         --defer-write \
         --minimize \
         --output-file optimal-frequency.config \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=20 \
         -- ./check_geopmopt_dgemm_ctl_run.sh optimal-frequency.config

# For DGEMM expect maximum frequency minimizes time in dgemm
NUM_FREQ=$(geopmgrid --cpu-frequency board --coordinate-range)
geopmgrid --cpu-frequency board --coordinate $(($NUM_FREQ-1)) > expected.config
grep ${MAX_FREQ} optimal-frequency.config
