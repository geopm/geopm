#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# test_kill_geopmd_batch_run.sh
# Original geopmd process is killed with signal 9 (requires root)

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating geopmd process
    --------------------------

    Terminate the original geopmd process and check that the
    batch server (owned by root) shuts down cleanly.

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

# TERMINATE THE GEOPMD PROCESS, VERIFY THAT THE BATCH SERVER AND THE CLIENT ARE GONE TOO
sleep 5
get_server_pid

kill_geopmd

sleep 5
check_server_dead
check_client_dead

restore_control

cleanup_resources

echo "SUCCESS"
exit 0
