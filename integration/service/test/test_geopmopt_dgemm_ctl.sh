#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

geopmopt --verbosity=2 \
         --cpu-frequency board \
         --defer-write \
         --minimize \
         --output-file optimal-frequency.config \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=20 \
         --print-stdout \
         -- ./check_geopmopt_dgemm_ctl_run.sh optimial-frequency.config

# For DGEMM expect maximum frequency minimizes time in dgemm
NUM_FREQ=$(geopmgrid --cpu-frequency board --coordinate-range)
geopmgrid --cpu-frequency board --coordinate $(($NUM_FREQ-1)) > expected.config
diff expected.config optimal-frequency.config
