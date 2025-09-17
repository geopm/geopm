#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Restarting the Service
    ----------------------

    Stop the service while the user has a write session open.

"
    exit 0
fi

SYSTEMD_VERSION=$(systemctl --version | grep systemd | awk '{print $2}')
if [[ ${SYSTEMD_VERSION} -lt 244 ]]; then
    echo "Warning: Skipping $0 because systemd does not support RestartKillSignal" 1>&2
    exit 0
fi

# PARAMETERS
CONTROL=MSR::PERF_CTL:FREQ
TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=${GEOPM_TEST_USER:-test-service}
if [[ $(whoami) == 'root' ]]; then
    TEST_SCRIPT="su ${TEST_USER} ${TEST_DIR}/check_write_session.sh"
else
    TEST_SCRIPT=${TEST_DIR}/check_write_session.sh
fi

# RUN WRITE SESSION TEST AND MAKE SURE IT PASSES
${TEST_SCRIPT} &
test_pid=$!
sleep 2
sudo systemctl restart geopm
wait $test_pid
result=$?
if [ $result -eq 0 ]; then
    echo "SUCCESS"
fi
exit $result
