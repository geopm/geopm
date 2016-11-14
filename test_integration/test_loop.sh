#!/bin/bash

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
    ./geopm_test_integration.py -fv > >(tee -a ${LOG_FILE}) 2>&1
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

ERR_MSG="The integration tests have failed at iteration ${COUNT}.  Please see the output for more information:\nhttp://10.23.186.193/~test/cron_runs/${TIMESTAMP}"

echo -e ${ERR_MSG} | mail -r "do-not-reply" -s "Integration test failure : ${TIMESTAMP}" brad.geltz@intel.com

echo "Done."
