#!/bin/bash
TIMESTAMP=$(date +%s)
LOG=/tmp/geopm-client-${TIMESTAMP}.log
sleep 60
echo Starting GEOPM Client: $(date) >> ${LOG}
# Read the TIME signal from the GEOPM service every second for an hour
echo "SERVICE::TIME board 0" | geopmsession -p 1 -t 3600 >> ${LOG}
