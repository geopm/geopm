#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# test_kill_batch_client.sh
# Session client PID is killed with signal 9 after
# the client pid is isolated from the parent process group using setsid(2)

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating Batch Client
    ------------------------

    Session client PID is killed with signal 9 after
    the client pid is isolated from the parent process group using setsid(2)
    Check that the batch server (owned by root) shuts down cleanly.

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

# TERMINATE THE CLIENT PROCESS, VERIFY THAT THE BATCH SERVER IS GONE TOO
sleep 5
get_server_pid

kill_client
sleep 5
check_server_dead

restore_control

cleanup_resources

echo "SUCCESS"
exit 0
