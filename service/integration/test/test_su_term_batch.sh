#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating Batch Server
    ------------------------

    As a user, send term signal to a geopmsession process owned by the
    test user, and make sure that server (owned by root) shuts down
    cleanly.

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

TEST_SCRIPT="${TEST_DIR}/test_su_term_batch_helper.sh"
if [[ $(whoami) == 'root' ]]; then
    export SESSION_PID=$(sudo -E -u ${TEST_USER} ${TEST_SCRIPT})
else
    export SESSION_PID=$(${TEST_SCRIPT})
fi

sleep 5
BATCH_PID=$(sudo geopmaccess -s ${SESSION_PID} | \
            python3 -c 'import sys,json; print(json.loads(sys.stdin.read())["batch_server"])')
kill -7 $SESSION_PID # Ok to send as test user
sleep 5
if ps --pid $BATCH_PID >& /dev/null; then
    kill -9 $BATCH_PID
    test_error "Batch server persists after client is terminated"
fi

echo "SUCCESS"
exit 0
