#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Script to be run with cron to periodically generate GEOPM reports on energy,
# power, frequency and temperature.
#
# Run this script as a cron job.  An example cron entry could be:
#
# */5 * * * * /path/to/geopm_report_cron.sh 300 1 /output/path/geopm-report &>> /output/path/geopm-report.log
# * 1 */1 * * /path/to/geopm_report_cron.py --static-html /output/path/geopm-report/$(date --date=yesterday '+%Y-%m-%d').html /output/path/geopm-report/$(date --date=yesterday '+%Y-%m-%d')/geopm-report-*.yml
#
# In the above example, the cron job runs every 5 minutes (*/5) and the sampler
# runs for 300 seconds (i.e. 5 minutes).  The reports generated in this way may
# be visualised with the test_cron_use_case_plot.py.  The second cron job runs
# nightly to generate a static visualization of the previous day's data in a
# static html page. Note that /path/to and /output/path are intended as place
# holders for paths that reflect the user's environment. To run the plotting
# script to create a web server just pass a list of report files to the command
# line interface without the --static-html option.
#
# python3 geopm_report_cron_plot.py /output/path/geopm-report/*/*.yml
#
# The output can be visualized by opening a web page to port 10850 on the server
# running the plotting script.

set -e

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 REPORT_PERIOD SAMPLE_PERIOD OUTPUT_DIR"
    echo "       REPORT_PERIOD: Period of time in seconds covered by each report"
    echo "       SAMPLE_PERIOD: Period of time in seconds (may be fractional) between telemetry samples"
    echo "       OUTPUT_DIR: Directory where reports are generated, a subdirectory is created for each day"
    exit -1
fi

REPORT_PERIOD=$1
SAMPLE_PERIOD=$2
OUTPUT_DIR=$3
DATE_TIME=$(date '+%F-%T-%Z')
DATE=$(echo $DATE_TIME | awk -F- '{print $1"-"$2"-"$3}')
OUTPUT_DIR=${OUTPUT_DIR}/${DATE}
mkdir -p ${OUTPUT_DIR}
for nn in $(geopmread | grep "POWER\|ENERGY\|FREQ\|TEMP" | \
                        grep -v '::\|CONTROL\|MAX\|MIN\|STEP\|LIMIT\|STICKER'); do
    echo ${nn} board 0
done | \
geopmsession --trace-out /dev/null \
             -r ${OUTPUT_DIR}/geopm-report-${DATE_TIME}.yml \
             -p ${SAMPLE_PERIOD} \
             -t ${REPORT_PERIOD}
