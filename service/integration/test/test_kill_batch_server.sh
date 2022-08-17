#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# test_kill_batch_server.sh
# Session batch server PID is killed with signal 9
# (requires root, so special script may need to be installed for this purpose)

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating Batch Server
    ------------------------

    Session batch server PID is killed with signal 9
    (requires root, so special script may need to be installed for this purpose)
    Check that the batch client shuts down cleanly.

"
    exit 0
fi

TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=test

source ${TEST_DIR}/save_restore.sh
source ${TEST_DIR}/process_server_actions.sh

# PARAMETERS
set_initial_parameters

set -x

save_control

run_batch_helper
sleep 5

check_control

# TERMINATE THE BATCH SERVER PROCESS, VERIFY THAT THE CLIENT RECEIVES AN ERROR
sleep 5

kill_server

sleep 5

if ! grep "The server is unresponsive" "${TEST_ERROR}"; then
    test_error "The error was not captured"
fi

check_client_dead

restore_control

cleanup_resources

echo "SUCCESS"
exit 0
