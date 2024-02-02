#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

export SCRIPT_DIR=$(dirname $(realpath $0))

set -x
set -e

# 10 second runtime
LOOP_COUNT=1000
NUM_TRIAL=10
DELAY=10e-3
PREFIX=test-batch-perf

# Clean environment and restart service
sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
sudo systemctl restart geopm

for ((TRIAL=0; TRIAL < NUM_TRIAL; TRIAL++)) do

    # Use mrs-safe driver through the GEOPM Service shared memory interface
    ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-safe-service-${TRIAL}.csv
    ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-safe-service-${TRIAL}.csv
    # Use mrs-safe driver directly without GEOPM Service
    sudo ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-safe-root-${TRIAL}.csv
    sudo chown ${USER} ${PREFIX}-msr-safe-root-${TRIAL}.csv

    sudo systemctl set-environment GEOPM_DISABLE_MSR_SAFE=TRUE
    sudo systemctl restart geopm

    # Use stock msr driver through the GEOPM Service with kernel asynchronous IO
    ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-uring-service-${TRIAL}.csv
    # Use stock msr driver directly with kernel asynchronous IO
    sudo env GEOPM_DISABLE_MSR_SAFE=TRUE ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-uring-root-${TRIAL}.csv

    sudo systemctl set-environment GEOPM_DISABLE_IO_URING=TRUE
    sudo systemctl restart geopm

    # Use stock msr driver through the GEOPM Service with serial reads
    ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-sync-service-${TRIAL}.csv
    # Use stock msr driver directly with serial reads
    sudo env GEOPM_DISABLE_MSR_SAFE=TRUE GEOPM_DISABLE_IO_URING=TRUE ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-sync-root-${TRIAL}.csv
    sudo chown ${USER} ${PREFIX}-msr-safe-root-${TRIAL}.csv

    # Restore environment
    sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
    sudo systemctl restart geopm

done

python3 ${SCRIPT_DIR}/plot_batch_perf.py --plot-path=test_batch_perf.png --min-y=1e-4 ${PREFIX}*.csv
