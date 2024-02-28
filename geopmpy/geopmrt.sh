#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ ! $GEOPM_SOURCE ]]; then
    echo "Error: set GEOPM_SOURCE environment variable" 1>&2
    exit -1
fi

PROFILE="$1"
PORT=8080
HOST=$(hostname -I | awk '{print $1}')
export LD_LIBRARY_PATH=$GEOPM_SOURCE/libgeopm/.libs:$GEOPM_SOURCE/libgeopmd/.libs:$LD_LIBRARY_PATH
export PYTHONPATH=$GEOPM_SOURCE/geopmpy:$GEOPM_SOURCE/service:$PYTHONPATH

https_proxy= \
$GEOPM_SOURCE/libgeopm/.libs/geopmrtd $HOST:$PORT &
https_proxy= \
$GEOPM_SOURCE/geopmpy/geopmrt --runtime-server $HOST:$PORT \
			      --database /tmp/geopmrt.db \
			      --agent monitor \
                              --profile "$PROFILE"

wait

