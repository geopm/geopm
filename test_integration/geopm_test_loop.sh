#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

# TODO Trap CTRL-C?

COUNT=0
TIMESTAMP=$(date +\%F_\%H\%M)
TEST_DIR=${HOME}/public_html/cron_runs/${TIMESTAMP}
LOG_FILE=test_loop_output.log

while [ $? -eq 0 -a ${COUNT} -lt 10 ];
do
    git clean -fd .
    COUNT=$((COUNT+1))
    echo "Beginning loop ${COUNT}..." > >(tee -a ${LOG_FILE})
    GEOPM_RUN_LONG_TESTS=true ./geopm_test_integration.py -fv > >(tee -a ${LOG_FILE}) 2>&1
done
TEST_RETURN_CODE=$?

# Save the output in a timestamped dir
echo "Moving files to ${TEST_DIR}..."
mkdir -p ${TEST_DIR}
for f in $(git ls-files --others --exclude-standard);
do
    mv ${f} ${TEST_DIR}
done
mv ${LOG_FILE} ${TEST_DIR}

#Do email only if there was a failure
if [ ${TEST_RETURN_CODE} -eq 0 ]; then
    exit 0
fi

ERR_MSG="The integration tests have failed at iteration ${COUNT}.  Please see the output for more information:\nhttp://$(hostname -i)/~test/cron_runs/${TIMESTAMP}"

echo -e ${ERR_MSG} | mail -r "do-not-reply" -s "Integration test failure : ${TIMESTAMP}" brad.geltz@intel.com,christopher.m.cantalupo@intel.com,steve.s.sylvester@intel.com,brandon.baker@intel.com

echo "Done."
