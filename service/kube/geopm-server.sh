#!/bin/bash
TIMESTAMP=$(date +%s)
LOG=/tmp/geopm-server-${TIMESTAMP}.log
echo Starting GEOPM Server: $(date) >> ${LOG}

# Enable users to access the TIME signal through the service
echo "TIME" | geopmaccess -w >> ${LOG}
