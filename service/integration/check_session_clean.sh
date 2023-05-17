#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# check_session_clean.sh
# Check that all the files in the session are cleaned up after
# the server shuts down.

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

check_file() {
    if [ -e $1 ]; then
        echo "Error: $1 file is not cleaned out." 1>&2
        if [ ${USER} == "root" ]; then
            rm -f $1
        else
            echo "Warning: File $1 not cleaned up and script not run as root" 1>&2
        fi
        fail=true
    fi
}

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Check Session Clean
    -------------------

    Check that all the files in the session are cleaned up after
    the server shuts down.
    If any of the files are not cleaned up, the script exits with
    an error, and cleans up the files in the session manually.

    The first argument should be the Session PID.

"
    exit 0
fi

if [[ $# -ne 1 ]]; then
    echo "Error: Session PID not specified."
    exit 0
fi

SESSION_PID=$1

SESSION_FILE="/run/geopm-service/session-${SESSION_PID}.json"
# fifo files for the batch status
BATCH_STATUS_IN="/run/geopm-service/batch-status-${SESSION_PID}-in"
BATCH_STATUS_OUT="/run/geopm-service/batch-status-${SESSION_PID}-out"
# shared memory keys
SHMEM_KEY_SIGNAL="/run/geopm-service/batch-buffer-${SESSION_PID}-signal"
SHMEM_KEY_CONTROL="/run/geopm-service/batch-buffer-${SESSION_PID}-control"
SHMEM_KEY_STATUS="/run/geopm-service/profile-$(id -u)-status"
SHMEM_KEY_RECORD_LOG="/run/geopm-service/profile-${SESSION_PID}-record-log"

check_file ${SESSION_FILE}
check_file ${BATCH_STATUS_IN}
check_file ${BATCH_STATUS_OUT}
check_file ${SHMEM_KEY_SIGNAL}
check_file ${SHMEM_KEY_CONTROL}
check_file ${SHMEM_KEY_STATUS}
check_file ${SHMEM_KEY_RECORD_LOG}

if [ ! -z "${fail}" ]; then
    exit -1
else
    exit 0
fi
