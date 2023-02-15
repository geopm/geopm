#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# test_systemctl_stop_geopm.sh
# Call systemctl stop geopm while a batch server is running and
# check that the client ends, the server ends, and the restore occurs

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating geopm using systemctl stop
    --------------------------------------

    Call systemctl stop geopm while a batch server is running and
    check that the client ends, the server ends, and the restore occurs

"
    exit 0
fi

TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=${GEOPM_TEST_USER:-test-service}

source ${TEST_DIR}/save_restore.sh
source ${TEST_DIR}/process_server_actions.sh

# PARAMETERS
set_initial_parameters

set -x

save_control

run_batch_helper
sleep 5

check_control

# TERMINATE THE GEOPMD PROCESS, VERIFY THAT THE BATCH SERVER AND THE CLIENT ARE GONE TOO
sleep 5
get_server_pid

systemctl_stop_geopm

sleep 5
# check that the server ends
check_server_dead

# check that the client ends
check_client_dead

systemctl_start_geopm
sleep 1

restore_control

cleanup_resources

echo "SUCCESS"
exit 0
