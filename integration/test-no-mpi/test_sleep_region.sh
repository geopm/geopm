#!/bin/bash

source ${GEOPM_SOURCE}/integration/config/run_env.sh 
TEST_NAME=test_sleep_region
export GEOPM_REPORT=${TEST_NAME}_report.yaml
export GEOPM_TRACE=${TEST_NAME}_trace.csv
export GEOPM_PROFILE=${TEST_NAME}
geopmctl &
sleep 2
./${TEST_NAME}

