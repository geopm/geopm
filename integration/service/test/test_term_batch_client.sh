#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating Batch Client
    ------------------------

    As root, send term signal to active batch server client, and make sure
    that server shuts down cleanly.

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

term_client
sleep 5
check_server_dead

restore_control

cleanup_resources

echo "SUCCESS"
exit 0
