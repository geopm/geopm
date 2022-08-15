#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Terminating Batch Server
    ------------------------

    As a user, send a term signal to a running python script owned by
    the test user that creates a writing batch server. Check that the
    batch server (owned by root) shuts down cleanly.

"
    exit 0
fi

# PARAMETERS
CONTROL=MSR::PERF_CTL:FREQ
STICKER=CPU_FREQUENCY_STICKER
DOMAIN=core
DOMAIN_IDX=0
REQUEST="${CONTROL} ${DOMAIN} ${DOMAIN_IDX}"
REQUEST_STICKER="${STICKER} board ${DOMAIN_IDX}"

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=test

set -x

# READ START VALUE OF CONTROL REGISTER
START_VALUE=$(geopmread ${REQUEST}) ||
    test_error "Call to read ${CONTROL} through geopmread failed"

# READ START VALUE OF STICKER REGISTER
REQUEST_VALUE=$(geopmread ${REQUEST_STICKER}) ||
    test_error "Call to read ${STICKER} through geopmread failed"

REQUEST_VALUE=$(( "$REQUEST_VALUE" - 100000000)) ||
    test_error "Attempt to adjust ${REQUEST_VALUE} failed"

# CHECK THAT IT IS DIFFERENT THAN THE TEST VALUE
test ${REQUEST_VALUE} != ${START_VALUE} ||
    test_error "Start value for the control is the same as the test value"

# START A PYTHON SCRIPT THAT WRITES THE CONTROL VALUE
TEST_SCRIPT="${TEST_DIR}/test_su_term_batch_write_helper.sh"
if [[ $(whoami) == 'root' ]]; then
    # sudo -u is used to change from the root user to the test user,
    # who does not have elevated privileges.
    TEMP_FILE=$(mktemp --tmpdir test_su_term_batch_write_XXXXXXXX.tmp)
    sudo -b -E -u ${TEST_USER} setsid ${TEST_SCRIPT} > ${TEMP_FILE}
    sleep 2
    export SESSION_PID=$(cat ${TEMP_FILE})
    rm ${TEMP_FILE}
else
    setsid python3 ${TEST_DIR}/test_su_term_batch_write_helper.py &
    export SESSION_PID=$!
fi
sleep 5

# READ THE CONTROLLED REGISTER
SESSION_VALUE=$(geopmread ${REQUEST})

# CHECK THAT THE VALUE OF THE CONTROL REGISTER HAS BEEN CHANGED
test ${SESSION_VALUE} == ${REQUEST_VALUE} ||
    test_error "Writing the new value has failed"

# TERMINATE THE CLIENT PROCESS, VERIFY THAT THE BATCH SERVER IS GONE TOO
sleep 5
# geopmaccess returns a JSON object containing the PID of the batch server
BATCH_PID=$(sudo get_batch_server.py ${SESSION_PID})

kill -7 $SESSION_PID # Ok to send as test user
sleep 5
if ps --pid $BATCH_PID >& /dev/null; then
    kill -9 $BATCH_PID
    test_error "Batch server persists after client is terminated"
fi

# READ THE RESTORED REGISTER
END_VALUE=$(geopmread ${REQUEST})

# CHECK THAT SAVE/RESTORE WORKED
test ${START_VALUE} == ${END_VALUE} ||
    test_error "Control is not restored after the session"

echo "SUCCESS"
exit 0
