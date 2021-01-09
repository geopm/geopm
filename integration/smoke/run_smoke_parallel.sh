#!/bin/bash
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

source smoke_env.sh

APPS_2NODE="dgemm_tiny dgemm nasft nekbone hpcg pennant hpl_mkl hpl_netlib"
# apps with no 2-node config
APPS_1NODE="minife amg"
ALL_EXP="monitor power_sweep power_balancer_energy frequency_sweep barrier_frequency_sweep uncore_frequency_sweep"

function save_test_result_line {
    # Logs the result of a run.
    # TODO: a future patch can use calls to script that saves in the DB, and
    #       might want to support arbitrary strings for result
    #
    # Inputs:
    #   SLURM_JOBID: Slurm job id from the job environment
    #   APP: name of the application
    #   EXP_TYPE: experiment type string
    #
    # Returns:
    #   PASS_RESULT: command to be run at the end of the slurm job to
    #                log the result with PASS
    #   FAIL_RESULT: command to be run at the end of the slurm job to
    #                log the result with FAIL
    #   SKIP_RESULT: command to be run at the end of the slurm job to
    #                log the result with SKIP

    local LOG_NAME="smoke_test_run_results.log"

    PASS_RESULT="echo \"\${SLURM_JOBID} ${APP} ${EXP_TYPE} PASS\" >> ${LOG_NAME}"
    FAIL_RESULT="echo \"\${SLURM_JOBID} ${APP} ${EXP_TYPE} FAIL\" >> ${LOG_NAME}"
    SKIP_RESULT="echo \"${APP} ${EXP_TYPE} SKIP\" >> ${LOG_NAME}"
}

function gen_sbatch {
    # Creates a slurm batch script to run the given smoke test experiment.
    #
    # TODO: This function has some copied code from
    # integration/experiment/gen_slurm.sh
    #
    # Inputs:
    #   NUM_NODES: number of nodes to use for the job
    #   APP: name of the application in integration/apps
    #   EXP_TYPE: experiment type string
    #
    # Returns:
    #   SBATCH_NAME: name of the sbatch script to be submitted, or
    #                empty if the run script is missing
    #   RUN_SCRIPT: name of the experiment run script for the given
    #               app and experiment type

    local EXP_DIR=${EXP_TYPE}
    if [ "${EXP_TYPE}" == "barrier_frequency_sweep" ] ||
       [ "${EXP_TYPE}" == "power_balancer_energy" ]; then
        EXP_DIR="energy_efficiency"
    fi

    local EXP_ARGS=""
    if [ "${EXP_TYPE}" == "power_sweep" ] ||
       [ "${EXP_TYPE}" == "power_balancer_energy" ]; then
        EXP_ARGS="--min-power=190 --max-power=230"
    elif [ "${EXP_TYPE}" == "frequency_sweep" ] ||
         [ "${EXP_TYPE}" == "barrier_frequency_sweep" ]; then
        EXP_ARGS="--min-frequency=1.9e9 --max-frequency=2.0e9"
    elif [ "${EXP_TYPE}" == "uncore_frequency_sweep" ]; then
        EXP_ARGS="--min-frequency=1.9e9 --max-frequency=2.0e9 --min-uncore-frequency=2.1e9 --max-uncore-frequency=2.2e9"
    fi

    # set up commands for PASS_RESULT, FAIL_RESULT, and SKIP_RESULT
    save_test_result_line

    RUN_SCRIPT=${GEOPM_SOURCE}/integration/experiment/${EXP_DIR}/run_${EXP_TYPE}_${APP}.py
    if [ -f "${RUN_SCRIPT}" ]; then
        SBATCH_NAME=${APP}_${EXP_TYPE}_${NUM_NODES}.sbatch
        cat > ${SBATCH_NAME} << EOF
#!/bin/bash
#SBATCH -N ${NUM_NODES}
#SBATCH -o %j.out
#SBATCH -J ${APP}_${EXP_TYPE}
#SBATCH -t 00:30:00

source ${GEOPM_SOURCE}/integration/config/run_env.sh
OUTPUT_DIR=\${SLURM_JOBID}_\${SLURM_JOB_NAME}

${RUN_SCRIPT} \\
    --node-count=\${SLURM_NNODES} \\
    --output-dir=\${OUTPUT_DIR} \\
    ${EXP_ARGS} \\
    # end

result=\$?
if [ \$result -eq 0 ]; then
   ${PASS_RESULT}
else
   ${FAIL_RESULT}
fi

EOF
        # Remove blank lines
        uniq ${SBATCH_NAME} .${SBATCH_NAME}
        mv .${SBATCH_NAME} ${SBATCH_NAME}
    else
        SBATCH_NAME=""
    fi
}

for EXP_TYPE in $ALL_EXP; do

    NUM_NODES=2
    for APP in ${APPS_2NODE}; do
        gen_sbatch
        if [ ! -z $SBATCH_NAME ]; then
            sbatch ${SBATCH_NAME}
            echo "${SBATCH_NAME} submitted"
        else
            echo "${RUN_SCRIPT} not found; skipping"
            # TODO: danger!
            eval ${SKIP_RESULT}
        fi
    done

    NUM_NODES=1
    for APP in ${APPS_1NODE}; do
        gen_sbatch
        if [ ! -z $SBATCH_NAME ]; then
            sbatch ${SBATCH_NAME}
            echo "${SBATCH_NAME} submitted"
        else
            echo "${RUN_SCRIPT} not found; skipping"
            # TODO: danger!
            eval ${SKIP_RESULT}
        fi
    done

done
