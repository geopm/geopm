#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

test_error() {
    echo "Error: ${0} restoring GEOPM service state and unloading msr" 1>&2
    exit -1
}

cleanup() {
    local ensure_msr_module_is_loaded=$1
    sudo systemctl stop geopm
    if [ $ensure_msr_module_is_loaded -eq 0 ]; then
        sudo modprobe -r msr
    else
        sudo modprobe msr
    fi
    sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
    sudo systemctl start geopm
}

if lsmod | grep -q '^msr\>'; then
        LOAD_MSR_ON_EXIT=1
else
        LOAD_MSR_ON_EXIT=0
fi
trap "cleanup $LOAD_MSR_ON_EXIT" EXIT

SCRIPT_DIR=$(dirname $(realpath $0))

if [ -e /usr/sbin/test_batch_perf ]; then
    TEST_BATCH_PERF=/usr/sbin/test_batch_perf
else
    TEST_BATCH_PERF=${SCRIPT_DIR}/test_batch_perf
fi

set -x

# 10 second runtime
LOOP_COUNT=1000
NUM_TRIAL=10
DELAY=10e-3
PREFIX=test-batch-perf

# Clean environment and restart service
sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
sudo modprobe msr
sudo systemctl restart geopm

for ((TRIAL=0; TRIAL < NUM_TRIAL; TRIAL++)) do

    # Use mrs-safe driver through the GEOPM Service shared memory interface
    ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-safe-service-${TRIAL}.csv || test_error
    ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-safe-service-${TRIAL}.csv || test_error
    # Use mrs-safe driver directly without GEOPM Service
    sudo ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-safe-root-${TRIAL}.csv || test_error
    sudo chown ${USER} ${PREFIX}-msr-safe-root-${TRIAL}.csv

    sudo systemctl set-environment GEOPM_DISABLE_MSR_SAFE=TRUE
    sudo systemctl restart geopm

    # Use stock msr driver through the GEOPM Service with kernel asynchronous IO
    ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-uring-service-${TRIAL}.csv || test_error
    # Use stock msr driver directly with kernel asynchronous IO
    sudo env GEOPM_DISABLE_MSR_SAFE=TRUE ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-uring-root-${TRIAL}.csv || test_error

    sudo systemctl set-environment GEOPM_DISABLE_IO_URING=TRUE
    sudo systemctl restart geopm

    # Use stock msr driver through the GEOPM Service with serial reads
    ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-sync-service-${TRIAL}.csv || test_error
    # Use stock msr driver directly with serial reads
    sudo env GEOPM_DISABLE_MSR_SAFE=TRUE GEOPM_DISABLE_IO_URING=TRUE ${TEST_BATCH_PERF} ${LOOP_COUNT} ${DELAY} > ${PREFIX}-msr-sync-root-${TRIAL}.csv || test_error
    sudo chown ${USER} ${PREFIX}-msr-sync-root-${TRIAL}.csv

    # Restore environment
    sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
    sudo systemctl restart geopm

done

python3 ${SCRIPT_DIR}/plot_batch_perf.py --plot-path=test_batch_perf.png --min-y=1e-4 ${PREFIX}*.csv
