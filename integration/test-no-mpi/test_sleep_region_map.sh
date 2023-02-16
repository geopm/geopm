#!/bin/bash

source ${GEOPM_SOURCE}/integration/config/run_env.sh 
TEST_NAME=test_sleep_region_map
export GEOPM_REPORT=${TEST_NAME}_report.yaml
export GEOPM_TRACE=${TEST_NAME}_trace.csv
export GEOPM_TRACE_PROFILE=${TEST_NAME}_trace_profile.csv
export GEOPM_PROFILE=${TEST_NAME}
export GEOPM_AGENT=frequency_map
export GEOPM_POLICY=frequency_map_policy.json
POLICY_ARRAY=2.0e9,NaN,NaN,memory_region,1.3e9,compute_region,3.7e9,network_region,1.0e9
geopmagent -a frequency_map \
           -p ${POLICY_ARRAY} > ${GEOPM_POLICY}

geopmctl &
sleep 2
./test_sleep_region


