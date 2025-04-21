#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

mkdir -p ${TMPDIR}

INPUT_FILE=$(mktemp)
cat > ${INPUT_FILE} << "EOF"
{
    "loop-count": 50,
    "region": ["stream", "dgemm"],
    "big-o": [3.0, 30.0]
}
EOF

export SYSTEMD_BUS_TIMEOUT=600

TEST_NAME=test_multi_app
export GEOPM_PROFILE=${TEST_NAME}
export GEOPM_PROGRAM_FILTER=geopmbench,stress-ng
export LD_PRELOAD=libgeopm.so.2.2.0

# GEOPM_CTL_LOCAL=true \
GEOPM_REPORT=${TEST_NAME}_report.yaml \
GEOPM_REPORT_SIGNALS=TIME@package \
GEOPM_NUM_PROC=2 \
setsid geopmctl &

# geopmbench
numactl --cpunodebind=0 -- geopmbench ${INPUT_FILE} &

# stress-ng
numactl --cpunodebind=1 -- stress-ng --cpu 1 --timeout 120 &

wait
rm ${INPUT_FILE}
