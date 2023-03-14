#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

source ${GEOPM_SOURCE}/integration/config/run_env.sh 
TEST_NAME=test_sleep
export GEOPM_REPORT=${TEST_NAME}_report.yaml
export GEOPM_TRACE=${TEST_NAME}_trace.csv
export GEOPM_TRACE_PROFILE=${TEST_NAME}_trace_profile.csv
export GEOPM_PROFILE=${TEST_NAME}
geopmctl &
sleep 2
LD_PRELOAD=libgeopmload.so sleep 10

