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
        # Requirements for service testing
        srun -N${SLURM_NNODES} -- sudo /usr/sbin/install_service.sh $(cat ${GEOPM_SOURCE}/VERSION) ${USER}

        # Set initial access
        cat > grant_access.sh <<EOF
#!/bin/bash
sudo geopmaccess -a | sudo geopmaccess -w
sudo geopmaccess -ac | sudo geopmaccess -wc
EOF
        chmod +x grant_access.sh
        srun -N ${SLURM_NNODES} -- grant_access.sh

        python3 -m unittest discover \
                --top-level-directory ${GEOPM_SOURCE} \
                --start-directory ${GEOPM_SOURCE}/service/integration/test \
                --pattern 'test_*.py' \
                --verbose &> >(tee -a service_${LOG_FILE})
        ERR=$?

        if [ ${ERR} -eq 0 ]; then
            # Only uninstall the service if the tests were clean.
            # Otherwise leave the service in place so it can be debugged in its current state.
            srun -N${SLURM_NNODES} -- sudo /usr/sbin/install_service.sh --remove
        fi
    fi
    echo "TEST LOOPER: Loop ${COUNT} done." > >(tee -a ${LOG_FILE})
done

#Do email only if there was a failure
if [ ${ERR} -ne 0 ]; then
    touch .tests_failed
fi

echo "Test looper done."

