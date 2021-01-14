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

# Variables to configure the set of test cases run by the main loop
#
# Applications with a valid 2-node configuration
APPS_2NODE="dgemm_tiny dgemm nasft nekbone hpcg pennant hpl_mkl hpl_netlib"
# Applications with no 2-node configuration
APPS_1NODE="minife amg"
# Experiment types to be run
ALL_EXP="monitor power_sweep power_balancer_energy frequency_sweep barrier_frequency_sweep uncore_frequency_sweep"
# Name of file to save results
LOG_NAME="smoke_test_run_results.log"

function pass_result {
    # String to be inserted in a script to log a test case as
    # completed with PASS result.
    #
    # Inputs:
    #   $1: name of the application
    #   $2: type of experiment
    #   $3: Slurm job ID
    #   LOG_NAME (global): log file to append results to
    #
    # Returns: a line to be inserted in a script to log the result

    echo "echo \"${1} ${2} ${3} PASS\" >> ${LOG_NAME}"
}

function fail_result {
    # String to be inserted in a script to log a test case as
    # completed with FAIL result.
    #
    # Inputs:
    #   $1: name of the application
    #   $2: type of experiment
    #   $3: Slurm job ID
    #   LOG_NAME (global): log file to append results to
    #
    # Returns: a line to be inserted in a script to log the result

    echo "echo \"${1} ${2} ${3} FAIL\" >> ${LOG_NAME}"
}

function skip_result {
    # Logs a test case as skipped because the required run script is
    # missing.
    #
    # Inputs:
    #   $1: name of the application
    #   $2: type of experiment
    #   LOG_NAME (global): log file to append results to

    echo "${1} ${2} SKIP" >> ${LOG_NAME}
}

function run_script_name {
    # Generates the name of the run script for a given combination of
    # experiment type and application.
    #
    # Inputs:
    #   $1: name of the application
    #   $2: type of experiment
    #   GEOPM_SOURCE (global): root directory of the GEOPM source code
    #
    # Returns: name of the run script in integration/experiments that
    # corresponds to the given experiment type and application
    local EXP_DIR=${2}
    if [ "${2}" == "barrier_frequency_sweep" ] ||
       [ "${2}" == "power_balancer_energy" ]; then
        EXP_DIR="energy_efficiency"
    fi
    echo "${GEOPM_SOURCE}/integration/experiment/${EXP_DIR}/run_${2}_${1}.py"
}

function gen_sbatch {
    # Creates a slurm batch script to run the given smoke test
    # experiment.
    #
    # TODO: This function has some copied code from
    # integration/experiment/gen_slurm.sh
    #
    # Inputs:
    #   $1: name of the application
    #   $2: type of experiment
    #   $3: number of nodes to use for launch
    #   $4: name of the corresponding run script; should not be empty
    #
    # Returns: name of the sbatch script to be submitted

    if [ ! -f "${4}" ]; then
        echo "Expected run script not found: ${4}"
        exit 1
    fi

    # check for additional sbatch arguments
    if [ ! -z ${GEOPM_USER_ACCOUNT} ]; then
        local SBATCH_ACCOUNT_LINE="#SBATCH -A ${GEOPM_USER_ACCOUNT}"
    fi

    if [ ! -z ${GEOPM_SYSTEM_DEFAULT_QUEUE} ]; then
        local SBATCH_QUEUE_LINE="#SBATCH -p ${GEOPM_SYSTEM_DEFAULT_QUEUE}"
    fi

    local EXP_ARGS=""
    if [ "${2}" == "power_sweep" ] ||
       [ "${2}" == "power_balancer_energy" ]; then
        EXP_ARGS="--min-power=170 --max-power=250"
    elif [ "${2}" == "frequency_sweep" ] ||
         [ "${2}" == "barrier_frequency_sweep" ]; then
        EXP_ARGS="--min-frequency=1.5e9 --max-frequency=2.1e9"
    elif [ "${2}" == "uncore_frequency_sweep" ]; then
        EXP_ARGS="--min-frequency=1.9e9 --max-frequency=2.1e9 --min-uncore-frequency=2.1e9 --max-uncore-frequency=2.2e9"
    fi

    local SBATCH_NAME=${1}_${2}_${3}.sbatch
    cat > ${SBATCH_NAME} << EOF
#!/bin/bash
#SBATCH -N ${NUM_NODES}
#SBATCH -o %j.out
#SBATCH -J ${1}_${2}
#SBATCH -t 12:30:00
${SBATCH_QUEUE_LINE}
${SBATCH_ACCOUNT_LINE}
${GEOPM_SBATCH_EXTRA_LINES}

source ${GEOPM_SOURCE}/integration/config/run_env.sh
OUTPUT_DIR=\${SLURM_JOBID}_\${SLURM_JOB_NAME}

${RUN_SCRIPT} \\
    --node-count=\${SLURM_NNODES} \\
    --output-dir=\${OUTPUT_DIR} \\
    ${EXP_ARGS} \\
    # end

result=\$?
if [ \$result -eq 0 ]; then
   $(pass_result ${1} ${2} \${SLURM_JOB_ID})
else
   $(fail_result ${1} ${2} \${SLURM_JOB_ID})
fi

EOF
    # Remove blank lines
    uniq ${SBATCH_NAME} .${SBATCH_NAME}
    mv .${SBATCH_NAME} ${SBATCH_NAME}

    echo ${SBATCH_NAME}
}

function try_test_case {
    # Generate a slurm script for the test case and launch it, or skip
    # the test case if an experiment script is not available for the
    # given application.
    #
    # Inputs:
    #   $1: name of the application
    #   $2: type of experiment
    #   $3: number of nodes to use for launch

    local RUN_SCRIPT=$(run_script_name ${1} ${2})
    if [ -f ${RUN_SCRIPT} ]; then
        local SBATCH_NAME=$(gen_sbatch ${1} ${2} ${3} ${RUN_SCRIPT})
        if [ $? -ne 0 ]; then
            exit 1
        fi
        sbatch ${SBATCH_NAME}
        echo "${SBATCH_NAME} submitted"
    else
        echo "${RUN_SCRIPT} not found; skipping"
        skip_result ${1} ${2}
    fi
}

# Main loop
#
# Tries to run every application with every experiment type, skipping
# when the corresponding script is not available in the
# integration/experiment directory.  Each test will be run in a
# separate slurm job that records its own PASS/FAIL status.
for EXP_TYPE in $ALL_EXP; do

    NUM_NODES=2
    for APP in ${APPS_2NODE}; do
        try_test_case ${APP} ${EXP_TYPE} ${NUM_NODES}
    done

    NUM_NODES=1
    for APP in ${APPS_1NODE}; do
        try_test_case ${APP} ${EXP_TYPE} ${NUM_NODES}
    done

done
