#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

cat > temp_config.json << "EOF"
{
    "loop-count": 2,
    "region": ["stream", "dgemm"],
    "big-o": [1.0, 10.0]
}
EOF

source ${GEOPM_SOURCE}/integration/config/run_env.sh

TEST_NAME=test_multi_app
export GEOPM_PROFILE=${TEST_NAME}
export GEOPM_PROGRAM_FILTER=geopmbench,stress-ng
export LD_PRELOAD=libgeopm.so.2.1.0

GEOPM_REPORT=${TEST_NAME}_report.yaml \
GEOPM_TRACE=${TEST_NAME}_trace.csv \
GEOPM_TRACE_PROFILE=${TEST_NAME}_trace_profile.csv \
GEOPM_REPORT_SIGNALS=TIME@package \
GEOPM_NUM_PROC=2 \
GEOPM_CTL_LOCAL=true \
setsid geopmctl &
sleep 1
# Check that second controller startup fails with correct error message
TEMP_LOG=$(mktemp)
geopmctl &> ${TEMP_LOG}
if [ $? -eq 0 ]; then
    echo "Error: Attempt to create second GEOPM Controller succeeded" 1>&2
    cat ${TEMP_LOG}
    rm ${TEMP_LOG}
    exit -1
fi
if ! grep -q "requested control of application profile, but the geopm service already has an application profile controlling client" ${TEMP_LOG}; then
    cat ${TEMP_LOG} 1>&2
    echo "Error: Error message did not match expected" 1>&2
    rm ${TEMP_LOG}
    exit -1
fi

# geopmbench
numactl --cpunodebind=0 -- geopmbench temp_config.json &

# stress-ng
numactl --cpunodebind=1 -- stress-ng --cpu 1 --timeout 10 &

wait $(jobs -p)
