#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Creating a write session:
    -------------------------

    This test shows the GEOPM service being used to set the maximum
    CPU frequency of core zero to 2 GHz.  The test uses the geopmread
    and geopmwrite command line tools to read and write from/to the
    MSR_PERF_CTL register to control the maximum frequency of the
    core.  The test first reads the value of the register, then opens
    a write session to set it to 2 GHz and reads the register again.
    The test then kills the write session with signal 9 and reads the
    control register a third time.  The test asserts that the control
    value was changed by the write session, and that this change was
    reverted to the value it started with after the session is
    killed.
"
    exit 0
fi

# PARAMETERS
CONTROL=MSR::PERF_CTL:FREQ
DOMAIN=core
DOMAIN_IDX=0
SETTING=2000000000
REQUEST="${CONTROL} ${DOMAIN} ${DOMAIN_IDX}"

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

# READ START VALUE OF CONTROL REGISTER
START_VALUE=$(geopmread ${REQUEST}) ||
    test_error "Call to read ${CONTROL} through geopmread failed"

# CHECK THAT IT IS DIFFERENT THAN THE TEST VALUE
test ${SETTING} != ${START_VALUE} ||
    test_error "Start value for the control is the same as the test value"

# START A SESSION THAT WRITES THE CONTROL VALUE
setsid $(dirname ${0})/do_write.sh SERVICE::${REQUEST} ${SETTING} &
SESSION_ID=$!
sleep 4

# READ THE CONTROLLED REGISTER
SESSION_VALUE=$(geopmread ${REQUEST})

wait ${SESSION_ID} ||
    test_error "Call to geopmwrite failed"
sleep 1

# READ THE RESTORED REGISTER
END_VALUE=$(geopmread ${REQUEST})

# CHECK THAT THE REGISTER WAS CHANGED DURING THE SESSION
test ${SETTING} == ${SESSION_VALUE} ||
    test_error "Control is not set during the session"

# CHECK THAT SAVE/RESTORE WORKED
test ${START_VALUE} == ${END_VALUE} ||
    test_error "Control is not restored after the session"

echo "SUCCESS"
exit 0
