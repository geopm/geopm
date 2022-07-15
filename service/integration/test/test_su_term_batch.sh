#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating Batch Server
    ------------------------

    As root, send term signal to active batch server client, and make sure
    that server shuts down cleanly.

"
    exit 0
fi

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=test

set -x

export SESSION_PID=$(sudo -E -u ${TEST_USER} ${TEST_DIR}/test_su_term_batch_helper.sh)
sleep 5
BATCH_PID=$(python3 ${TEST_DIR}/test_su_term_batch_helper.py ${SESSION_PID})
kill -7 $SESSION_PID
sleep 5
if ps --pid $BATCH_PID >& /dev/null; then
    test_error "Batch server persists after client is terminated"
    kill -9 $BATCH_PID
fi

echo "SUCCESS"
exit 0
