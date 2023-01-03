#!/bin/bash
set -x
echo Starting GEOPM Server: $(date)
BASE_DIR=/run/geopm-service
mkdir -p -m 711 ${BASE_DIR}
chmod 711 ${BASE_DIR}
echo "TIME" | geopmaccess --write --direct --force
sleep 5
ls -l ${BASE_DIR}
CONFIG_DIR=/etc/geopm-service
ls -ld ${CONFIG_DIR}
cat ${CONFIG_DIR}/0.DEFAULT_ACCESS/allowed_signals
nohup geopmd --grpc &>> /tmp/geopmd.log < /dev/null &
sleep 1
ps -aux
