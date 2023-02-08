#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# test_kill_geopmd_serial_run.sh
# Send signal 9 to geopmd while serial write has occurred and client is sleeping,
# check that the client stays alive, no restore occurs,
# and the client is able to write once the service is automatically restarted

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating geopmd process serial write
    ---------------------------------------

    Send signal 9 to geopmd while serial write has occurred and client is sleeping,
    check that the client stays alive, no restore occurs,
    and the client is able to write once the service is automatically restarted

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

run_serial_helper
sleep 1

check_control

kill_geopmd
# After we kill geopmd, client should still be around, and the server was never there
# because it is a serial run, not a batch (server) run.
sleep 7

check_client_alive

sleep 5

check_client_dead

restore_control

cleanup_resources

echo "SUCCESS"
exit 0
