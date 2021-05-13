#!/bin/bash
#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    test_write_session.sh script.  Access to the control is granted
    and it is shown that the test user can succefully change the
    value.

"
    exit 0
fi

# PARAMETERS
CONTROL=MSR::PERF_CTL:FREQ
TEST_DIR=$(dirname $(readlink -f $0))
TEST_USER=test
TEST_SCRIPT=${TEST_DIR}/test_write_session.sh
SAVE_SIGNALS=test_su_give_access_save_signals.txt
SAVE_CONTROLS=test_su_give_access_save_controls.txt
TEST_SIGNALS=test_su_give_access_test_signals.txt
TEST_CONTROLS=test_su_give_access_test_controls.txt

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

# MAKE SURE TEST IS RUN AS ROOT USER
test ${USER} == "root" ||
    test_error "Must be run as the root user, this is an administrator test"

# SAVE INITIAL ACCESS SETTINGS
geopmaccess > ${SAVE_SIGNALS}
geopmaccess -c > ${SAVE_CONTROLS}

# REMOVE CONTROL FROM ACCESS LIST FOR READING AND WRITING
grep -v ${CONTROL} ${SAVE_SIGNALS} > ${TEST_SIGNALS}
grep -v ${CONTROL} ${SAVE_CONTROLS} > ${TEST_CONTROLS}
geopmaccess -w < ${TEST_SIGNALS}
geopmaccess -w -c < ${TEST_CONTROLS}

# RUN WRITE SESSION TEST AND MAKE SURE IT FAILS
su ${TEST_USER} ${TEST_SCRIPT} &&
    test_error "Access to $CONTROL was disabled, but write session passed"

# ADD THE CONTROL INTO ACCESS LIST FOR READING AND WRITING
echo ${CONTROL} >> ${TEST_SIGNALS}
echo ${CONTROL} >> ${TEST_CONTROLS}
geopmaccess -w < ${TEST_SIGNALS}
geopmaccess -w -c < ${TEST_CONTROLS}

# RUN WRITE SESSION TEST AND MAKE SURE IT PASSES
su ${TEST_USER} ${TEST_SCRIPT} ||
    test_error "Access to $CONTROL was enabled, but write session failed"

# RESTORE INITIAL ACCESS SETTINGS
geopmaccess -w < ${SAVE_SIGNALS}
geopmaccess -w -c < ${SAVE_CONTROLS}

# CLEAN UP TEMPORARY FILES
rm ${SAVE_SIGNALS} ${SAVE_CONTROLS} ${TEST_SIGNALS} ${TEST_CONTROLS}

echo "SUCCESS"
exit 0
