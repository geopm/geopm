#!/bin/bash
set -x
export GEOPM_DEBUG=True
echo Starting GEOPM Client: $(date)
sleep 10
ls -l /run/geopm-service/SESSION_BUS_SOCKET
geopmread
geopmread -d
geopmread TIME board 0
geopmread SERVICE::TIME board 0
echo "SERVICE::TIME board 0" | geopmsession -p 1 -t 3600
