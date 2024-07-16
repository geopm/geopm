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

export https_proxy=''
# Reset the test database
##$GEOPM_SOURCE/geopmpy/geopmpy/init_db.py /tmp/geopmrt.db

$GEOPM_SOURCE/geopmpy/geopmpy/runtime_logger.py /tmp/geopmrt.db &
logger_pid=$!
# TODO: modify the app to have a timeout so we don't need to sleep here
sleep 0.1

$GEOPM_SOURCE/libgeopm/.libs/geopmrtd $HOST:$PORT &
geopmrtd_pid=$!

$GEOPM_SOURCE/geopmpy/geopmrt --runtime-server $HOST:$PORT \
			      --agent monitor \
                              --profile "$PROFILE" \
                              http://localhost:5000 &
geopmrt_pid=$!


trap 'kill $logger_pid $geopmrtd_pid $geopmrt_pid; exit' INT

wait
