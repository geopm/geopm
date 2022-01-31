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
    Creating a write session:
    -------------------------

    This test shows the GEOPM service being used to set the maximum
    CPU frequency of core zero to 2 GHz.  The test uses the
    geopmsession command line tool to read and write from/to the
    MSR_PERF_CTL register to control the maximum frequency of the
    core.  The test first reads the value of the register, then opens
    a write session to set it to 2 GHz and reads the register again.
    The test then kills the write session with signal 9 and reads the
    control register a third time.  The test asserts that the control
    value was changed by the write sesssion, and that this change was
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
START_VALUE=$(echo ${REQUEST} | geopmsession) ||
    test_error "Call to read ${CONTROL} through geopmsession failed"

# CHECK THAT IT IS DIFFERENT THAN THE TEST VALUE
test ${SETTING} != ${START_VALUE} ||
    test_error "Start value for the control is the same as the test value"

# START A SESSION THAT WRITES THE CONTROL VALUE
echo "${REQUEST} ${SETTING}" | setsid geopmsession -w -t 10 &
SESSION_ID=$!
sleep 4

# READ THE CONTROLLED REGISTER
SESSION_VALUE=$(echo ${REQUEST} | geopmsession)

# END THE SESSION
kill -9 ${SESSION_ID} ||
    test_error "Failed to kill session"
sleep 1

# READ THE RESTORED REGISTER
END_VALUE=$(echo ${REQUEST} | geopmsession)

# CHECK THAT THE REGISTER WAS CHANGED DURING THE SESSION
test ${SETTING} == ${SESSION_VALUE} ||
    test_error "Control is not set during the session"

# CHECK THAT SAVE/RESTORE WORKED
test ${START_VALUE} == ${END_VALUE} ||
    test_error "Control is not restored after the session"

echo "SUCCESS"
exit 0
