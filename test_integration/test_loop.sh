#!/bin/bash

COUNT=0
TIMESTAMP=$(date +%F_%H%M)
TEST_DIR=/tmp/int_test_${TIMESTAMP}
LOG_FILE=test_loop_output.log

until [ $? -ne 0 ];
do
    git clean -fd .
    COUNT=$((COUNT+1))
    echo "Beginning loop ${COUNT}..." > >(tee -a ${LOG_FILE})
    ./geopm_test_integration.py -v > >(tee -a ${LOG_FILE}) 2>&1
done

# Save the output in a timestamped dir in tmp/
mkdir ${TEST_DIR}
for f in $(git ls-files --others --exclude-standard);
do
    mv ${f} ${TEST_DIR}
done
