#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

print_help() {
    echo "
    Usage: $0              - Kill main geopmd process
           $0 SESSION_PID  - Kill batch server process owned by session

    This helper script my be added to the suders file to give users running
    the integration tests permissions to kill geopmd processes.

"
    exit 0
}

kill_error() {
    echo "Error: $1" 1>&2
    exit -1
}


if [[ $1 == '--help' ]]; then
    print_help
elif [[ ${USER} != "root" ]]; then
    kill_error "Script must be run as user root"
elif [[ $# -eq 0 ]]; then
    MAIN_PID=$(systemctl show geopm --property=MainPID | awk -F= '{print $2}')
    if [ ! -z "${MAIN_PID}" ] && ps ${MAIN_PID} >& /dev/null; then
        kill -9 ${MAIN_PID}
    else
        kill_error "Error: Unable to parse main PID from systemctl"
    fi
else
    SESSION_PID=$1
    BATCH_PID=$(sudo get_batch_server.py ${SESSION_PID})
    if [ ! -z "${BATCH_PID}" ] && ps ${BATCH_PID} >& /dev/null; then
        kill -9 ${BATCH_PID}
    else
        kill_error "Error: Unable to parse batch server PID from geopmaccess"
    fi
fi
