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

# GEOPM SBATCH run script generator for SLURM

# Check command line args
if [ $# -eq 3 ]; then
    NUM_NODES=$1
    APP=$2
    EXP_DIR=$3
    EXP_TYPE=$3
elif [ $# -eq 4 ]; then
    NUM_NODES=$1
    APP=$2
    EXP_DIR=$3
    EXP_TYPE=$4
else
    echo "Usage:"
    echo "       $0 NUM_NODES APP EXPERIMENT_DIR [EXPERIMENT_TYPE]"
    echo
    echo "  EXPERIMENT_TYPE: Optional, required when the directory name does not match"
    echo "                   the Python experiment file."
    echo
    echo "Examples:"
    echo "  $ ./gen_slurm.sh 1 nekbone monitor"
    echo "  $ ./gen_slurm.sh 1 nekbone energy_efficiency power_balancer_energy"
    echo
    exit 1
fi

if [ -f $HOME/.geopmrc ]; then
    source ~/.geopmrc
fi
GEOPM_SOURCE=${GEOPM_SOURCE:?"Please define GEOPM_SOURCE in your environment"}
GEOPM_WORKDIR=${GEOPM_WORKDIR:?"Please define GEOPM_WORKDIR in your environment"}

if [ ! -z ${GEOPM_SYSTEM_ENV} ]; then
    source ${GEOPM_SYSTEM_ENV}
fi

# check for additional sbatch arguments
if [ ! -z ${GEOPM_SLURM_ACCOUNT} ]; then
    SBATCH_ACCOUNT_LINE="#SBATCH -A ${GEOPM_USER_ACCOUNT}"
fi

if [ ! -z ${GEOPM_SLURM_DEFAULT_QUEUE} ]; then
   SBATCH_QUEUE_LINE="#SBATCH -p ${GEOPM_SYSTEM_DEFAULT_QUEUE}"
fi

SCRIPT_NAME=test.sbatch
cat > ${SCRIPT_NAME} << EOF
#!/bin/bash
#SBATCH -N ${NUM_NODES}
#SBATCH -o %j.out
#SBATCH -D ${GEOPM_WORKDIR}
#SBATCH -J ${APP}_${EXP_TYPE}
#SBATCH -t 00:30:00
${SBATCH_QUEUE_LINE}
${SBATCH_ACCOUNT_LINE}
${GEOPM_SBATCH_EXTRA_LINES}

source ${GEOPM_SOURCE}/integration/config/run_env.sh
OUTPUT_DIR=\${GEOPM_WORKDIR}/\${SLURM_JOB_NAME}_\${SLURM_JOBID}

${GEOPM_SOURCE}/integration/experiment/${EXP_DIR}/run_${EXP_TYPE}_${APP}.py \\
    --node-count=\${SLURM_NNODES} \\
    --output-dir=\${OUTPUT_DIR} \\
    # end

EOF

uniq ${SCRIPT_NAME} .${SCRIPT_NAME}
mv .${SCRIPT_NAME} ${SCRIPT_NAME}

