#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex
geopmopt --verbosity=2 \
         --cpu-frequency=board \
         --cpu-uncore-frequency=board \
         --defer-write \
         --minimize \
         --output-file=optimal-frequency.config \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=50 \
         -- ./check_geopmopt_aib_run.sh optimal-frequency.config
