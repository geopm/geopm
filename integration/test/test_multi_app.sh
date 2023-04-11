#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

cat > temp_config.json << "EOF"
{
    "loop-count": 2,
    "region": ["stream", "dgemm"],
    "big-o": [1.0, 1.0]
}
EOF

source ${GEOPM_SOURCE}/integration/config/run_env.sh

TEST_NAME=test_multi_app
export GEOPM_REPORT=${TEST_NAME}_report.yaml
export GEOPM_TRACE=${TEST_NAME}_trace.csv
export GEOPM_TRACE_PROFILE=${TEST_NAME}_trace_profile.csv
export GEOPM_PROFILE=${TEST_NAME}
#export GEOPM_REPORT_SIGNALS=CPU_TIMESTAMP_COUNTER@package
export GEOPM_REPORT_SIGNALS=TIME@package

geopmctl &
sleep 2

# geopmbench
srun -N 1 -n 1 --cpu-bind=map_ldom:0 -s sh -c "LD_PRELOAD=libgeopmload.so.1.0.0 geopmbench temp_config.json" &

# stress-ng
srun -N 1 -n 1 --cpu-bind=map_ldom:1 -s sh -c "LD_PRELOAD=libgeopmload.so.1.0.0 stress-ng --cpu 1 --timeout 10" &

wait $(jobs -p)
