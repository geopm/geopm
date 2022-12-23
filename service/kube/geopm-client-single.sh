#!/bin/bash
TIMESTAMP=$(date +%s)
LOG=/tmp/geopm-client-single-${TIMESTAMP}.log
echo Starting GEOPM Client: $(date) >> ${LOG}
for ii in $(seq 0 10); do
    geopmread TIME board 0 >> ${LOG}
    sleep 1
done
