#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# --- Different ways of starting a process ---

run_batch_helper() {
    # START A PYTHON SCRIPT THAT WRITES THE CONTROL VALUE
    TEST_SCRIPT="${TEST_DIR}/batch_write_client_helper.sh"
    # the client pid is isolated from the parent process group using setsid(2)
    if [[ $(whoami) == 'root' ]]; then
        # sudo -u is used to change from the root user to the test user,
        # who does not have elevated privileges.
        TEMP_FILE=$(mktemp --tmpdir test_batch_write_client_XXXXXXXX.tmp)
        sudo -b -E -u ${TEST_USER} setsid ${TEST_SCRIPT} > ${TEMP_FILE} 2> ${TEST_ERROR}
        sleep 2
        export SESSION_PID=$(cat ${TEMP_FILE})
        rm ${TEMP_FILE}
    else
        setsid python3 ${TEST_DIR}/batch_write_client_helper.py 2> ${TEST_ERROR} &
        export SESSION_PID=$!
    fi
}

run_serial_helper() {
    # START A PYTHON SCRIPT THAT WRITES THE CONTROL VALUE
    TEST_SCRIPT="${TEST_DIR}/serial_write_client_helper.sh"
    # the client pid is isolated from the parent process group using setsid(2)
    if [[ $(whoami) == 'root' ]]; then
        # sudo -u is used to change from the root user to the test user,
        # who does not have elevated privileges.
        TEMP_FILE=$(mktemp --tmpdir serial_write_client_XXXXXXXX.tmp)
        sudo -b -E -u ${TEST_USER} setsid ${TEST_SCRIPT} > ${TEMP_FILE} 2> ${TEST_ERROR}
        sleep 2
        export SESSION_PID=$(cat ${TEMP_FILE})
        rm ${TEMP_FILE}
    else
        setsid python3 ${TEST_DIR}/serial_write_client_helper.py 2> ${TEST_ERROR} &
        export SESSION_PID=$!
    fi
}

# --- Different ways of killing a process ---

kill_client() {
    # Session client PID is killed with signal 9
    kill -9 $SESSION_PID # Ok to send as test user
}

kill_server() {
    # Session batch server PID is killed with signal 9
    # using the special script, because requires root privileges
    sudo kill_geopmd.sh ${SESSION_PID}
}

kill_geopmd() {
    # Original geopmd process is killed with signal 9 (requires root)
    sudo kill_geopmd.sh
}

term_client(){
    kill -7 $SESSION_PID
}

systemctl_stop_geopm() {
    # Call systemctl stop geopm while a batch server is running
    sudo systemctl stop geopm
    sleep 3
}

systemctl_start_geopm() {
    # Start the geopmd process again to restore the controls
    sudo systemctl start geopm
}

# --- Different ways of getting status about that process ---

check_client_alive() {
    if ! ps --pid $SESSION_PID >& /dev/null; then
        test_error "Client process is dead"
    fi
}

check_client_dead() {
    if ps --pid $SESSION_PID >& /dev/null; then
        kill -9 $SESSION_PID
        test_error "Client process persists after batch server is terminated"
    fi
}

check_server_dead() {
    if ps --pid $BATCH_PID >& /dev/null; then
        kill -9 $BATCH_PID
        test_error "Batch server persists after client is terminated"
    fi
}

get_server_pid() {
    sleep 2
    BATCH_PID=$(sudo get_batch_server.py ${SESSION_PID})
    echo "batch server pid is ${BATCH_PID}"
}
