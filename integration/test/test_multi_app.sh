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

numactl --cpunodebind=0 -- geopmbench temp_config.json &
numactl --cpunodebind=1 -- stress-ng --cpu 4 --timeout 10 &

wait $(jobs -p)
