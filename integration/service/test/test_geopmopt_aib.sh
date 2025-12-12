#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex
geopmopt --cpu-frequency=board \
         --cpu-uncore-frequency=board \
         --verbosity=2 \
         --defer-write \
         --minimize \
         --output-file=optimal-frequency-aib-${AIB_INTENSITY}.config \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --trials=40 \
         -- ./check_geopmopt_aib_run.sh optimal-frequency-aib-${AIB_INTENSITY}.config
