#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

COUNT=0
ERR=0
LOG_FILE=test_loop_output.log

while [ ${ERR} -eq 0 -a ${COUNT} -lt 10 ];
do
    COUNT=$((COUNT+1))
    echo "TEST LOOPER: Beginning loop ${COUNT}..." > >(tee -a ${LOG_FILE})
    python3 -m unittest discover \
            --top-level-directory ${GEOPM_SOURCE} \
            --start-directory ${GEOPM_SOURCE}/integration/test \
            --pattern 'test_*.py' \
            --verbose &> >(tee -a ${LOG_FILE})
    ERR=$?

    if [ ${ERR} -eq 0 ]; then
        echo "TEST LOOPER: Continuing loop ${COUNT} service tests..." > >(tee -a ${LOG_FILE})
        python3 -m unittest discover \
                --top-level-directory ${GEOPM_SOURCE} \
                --start-directory ${GEOPM_SOURCE}/service/integration/test \
                --pattern 'test_*.py' \
                --verbose &> >(tee -a service_${LOG_FILE})
        ERR=$?
    fi
    echo "TEST LOOPER: Loop ${COUNT} done." > >(tee -a ${LOG_FILE})
done

#Do email only if there was a failure
if [ ${ERR} -ne 0 ]; then
    touch .tests_failed
fi

echo "Test looper done."

