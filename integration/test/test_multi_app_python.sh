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

TEST_NAME=test_multi_app_python
export GEOPM_PROFILE=${TEST_NAME}

# GEOPM_CTL_LOCAL=true \
GEOPM_REPORT=${TEST_NAME}_report.yaml \
GEOPM_REPORT_SIGNALS=TIME@package \
GEOPM_NUM_PROC=2 \
setsid geopmctl &

# geopmbench
#   geopmbench already links against libgeopm, no LD_PRELOAD is needed
GEOPM_PROGRAM_FILTER=geopmbench numactl --cpunodebind=0 -- geopmbench ${INPUT_FILE} &

# python example
#   GEOPM @ v3.1 requires the use of LD_PRELOAD
LD_PRELOAD=libgeopm.so.2.2.0 GEOPM_PROGRAM_FILTER=python3 numactl --cpunodebind=1 -- python3 -c 'import time; time.sleep(120)' &
#   GEOPM @ v3.2 and beyond provides a gffi module to dl_open libgeopm; LD_PRELOAD is no longer necessary
# GEOPM_PROGRAM_FILTER=python3 python3 -c 'import geopmpy.gffi, time; time.sleep(120)' &

wait
rm ${INPUT_FILE}
