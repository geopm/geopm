#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# 10 second runtime
LOOP_COUNT=1000
DELAY=10e-3

# Clean environment and restart service
sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
sudo systemctl restart geopm

# Use mrs-safe driver through the GEOPM Service shared memory interface
${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > msr-safe-service.csv
# Use mrs-safe driver directly without GEOPM Service
sudo ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > msr-safe-root.csv
sudo chown ${USER} msr-safe-root.csv

sudo systemctl set-environment GEOPM_DISABLE_MSR_SAFE=TRUE
sudo systemctl restart geopm
# Use stock msr driver through the GEOPM Service with kernel asynchronous IO
${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > msr-uring-service.csv
# Use stock msr driver directly with kernel asynchronous IO
sudo env GEOPM_DISABLE_MSR_SAFE=TRUE ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > msr-uring-root.csv

sudo systemctl set-environment GEOPM_DISABLE_IO_URING=TRUE
sudo systemctl restart geopm

# Use stock msr driver through the GEOPM Service with serial reads
${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > msr-sync-service.csv
# Use stock msr driver directly with serial reads
sudo env GEOPM_DISABLE_MSR_SAFE=TRUE GEOPM_DISABLE_IO_URING=TRUE ${SCRIPT_DIR}/test_batch_perf ${LOOP_COUNT} ${DELAY} > msr-sync-root.csv
sudo chown ${USER} msr-safe-root.csv

# Restore environment
sudo systemctl unset-environment GEOPM_DISABLE_IO_URING GEOPM_DISABLE_MSR_SAFE
sudo systemctl restart geopm
