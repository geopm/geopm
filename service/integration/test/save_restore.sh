#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

set_initial_parameters() {
    CONTROL=MSR::PERF_CTL:FREQ
    STICKER=CPU_FREQUENCY_STICKER
    DOMAIN=core
    DOMAIN_IDX=0
    REQUEST="${CONTROL} ${DOMAIN} ${DOMAIN_IDX}"
    REQUEST_STICKER="${STICKER} board ${DOMAIN_IDX}"

    TEST_ERROR=$(mktemp --tmpdir test_error_output_XXXXXXXX.tmp)
}

save_control() {
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
}

check_control() {
    # READ THE CONTROLLED REGISTER
    SESSION_VALUE=$(geopmread ${REQUEST})

    # CHECK THAT THE VALUE OF THE CONTROL REGISTER HAS BEEN CHANGED
    test ${SESSION_VALUE} == ${REQUEST_VALUE} ||
        test_error "Writing the new value has failed: $(cat ${TEST_ERROR})"
}

restore_control() {
    # READ THE RESTORED REGISTER
    END_VALUE=$(geopmread ${REQUEST})

    # CHECK THAT SAVE/RESTORE WORKED
    test ${START_VALUE} == ${END_VALUE} ||
        test_error "Control is not restored after the session"
}

cleanup_resources() {
    rm "${TEST_ERROR}"
    if [ ! -z "${SESSION_PID}" ]; then
        sudo check_session_clean.sh ${SESSION_PID}
        test $? == 0 ||
            test_error "Session did not end cleanly, files persist in the run dir."
    fi
}
