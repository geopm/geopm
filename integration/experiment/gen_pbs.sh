#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GEOPM PBS run script generator

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
    echo "  $ ./gen_pbs.sh 1 nekbone monitor"
    echo "  $ ./gen_pbs.sh 1 nekbone energy_efficiency power_balancer_energy"
    echo "  $ GEOPM_SYSTEM_DEFAULT_QUEUE=R1259 ./gen_pbs.sh 1 arithmetic_intensity monitor"
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
if [ ! -z ${GEOPM_USER_ACCOUNT} ]; then
    PBS_ACCOUNT_LINE="#PBS -A ${GEOPM_USER_ACCOUNT}"
fi

if [ ! -z ${GEOPM_SYSTEM_DEFAULT_QUEUE} ]; then
    PBS_QUEUE_LINE="#PBS -q ${GEOPM_SYSTEM_DEFAULT_QUEUE}"
fi

SCRIPT_NAME=test.pbs
cat > ${SCRIPT_NAME} << EOF
#!/bin/bash
#PBS -l nodes=${NUM_NODES}
#PBS -o ${GEOPM_WORKDIR}/${APP}_${EXP_TYPE}.out
#PBS -j oe
#PBS -N ${APP}_${EXP_TYPE}
#PBS -l walltime=00:30:00
${PBS_QUEUE_LINE}
${PBS_ACCOUNT_LINE}
${GEOPM_PBS_EXTRA_LINES}

source ${GEOPM_SOURCE}/integration/config/run_env.sh
OUTPUT_DIR=\${GEOPM_WORKDIR}/\${PBS_JOBNAME}_\${PBS_JOBID}

cd \${GEOPM_WORKDIR}

${GEOPM_SOURCE}/integration/experiment/${EXP_DIR}/run_${EXP_TYPE}_${APP}.py \\
    --node-count=${NUM_NODES} \\
    --output-dir=\${OUTPUT_DIR} \\
    # end

EOF

uniq ${SCRIPT_NAME} .${SCRIPT_NAME}
mv .${SCRIPT_NAME} ${SCRIPT_NAME}

