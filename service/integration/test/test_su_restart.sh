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
    Restarting the Service
    ----------------------

    Stop the service while the user has a write session open.

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

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

# RUN WRITE SESSION TEST AND MAKE SURE IT PASSES
${TEST_SCRIPT} &
sleep 1
sudo systemctl stop geopm
sleep 1
sudo systemctl start geopm
wait

echo "SUCCESS"
exit 0
