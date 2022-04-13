#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Enabling User Access
    --------------------

    This test configures the GEOPM service to enable and disable any
    user to read and write to bits 8-15 of the MSR_PERF_CTL register
    which controls the maximum frequency of the core.  The test saves
    the existing settings so they can be restored at the end of the
    test.  Access to the control is removed from the list, and it is
    shown that the test user cannot succesfully run the
    check_write_session.sh script.  Access to the control is granted
    and it is shown that the test user can succefully change the
    value.

"
    exit 0
fi

# PARAMETERS
CONTROL=MSR::PERF_CTL:FREQ
TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=test
if [[ $(whoami) == 'root' ]]; then
    TEST_SCRIPT="su ${TEST_USER} ${TEST_DIR}/check_write_session.sh"
else
    TEST_SCRIPT=${TEST_DIR}/check_write_session.sh
fi
SAVE_SIGNALS=$(mktemp)
SAVE_CONTROLS=$(mktemp)
TEST_SIGNALS=$(mktemp)
TEST_CONTROLS=$(mktemp)

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

# SAVE INITIAL ACCESS SETTINGS
geopmaccess > ${SAVE_SIGNALS}
geopmaccess -c > ${SAVE_CONTROLS}

# REMOVE CONTROL FROM ACCESS LIST FOR READING AND WRITING
grep -v ${CONTROL} ${SAVE_SIGNALS} > ${TEST_SIGNALS}
grep -v ${CONTROL} ${SAVE_CONTROLS} > ${TEST_CONTROLS}
sudo -n geopmaccess -w < ${TEST_SIGNALS}
sudo -n geopmaccess -w -c < ${TEST_CONTROLS}

# RUN WRITE SESSION TEST AND MAKE SURE IT FAILS
${TEST_SCRIPT} &&
    test_error "Access to $CONTROL was disabled, but write session passed"

# ADD THE CONTROL INTO ACCESS LIST FOR READING AND WRITING
echo ${CONTROL} >> ${TEST_SIGNALS}
echo ${CONTROL} >> ${TEST_CONTROLS}
sudo -n geopmaccess -w < ${TEST_SIGNALS}
sudo -n geopmaccess -w -c < ${TEST_CONTROLS}

# RUN WRITE SESSION TEST AND MAKE SURE IT PASSES
${TEST_SCRIPT} ||
    test_error "Access to $CONTROL was enabled, but write session failed"

# RESTORE INITIAL ACCESS SETTINGS
sudo -n geopmaccess -w < ${SAVE_SIGNALS}
sudo -n geopmaccess -w -c < ${SAVE_CONTROLS}

# CLEAN UP TEMPORARY FILES
rm ${SAVE_SIGNALS} ${SAVE_CONTROLS} ${TEST_SIGNALS} ${TEST_CONTROLS}

echo "SUCCESS"
exit 0
