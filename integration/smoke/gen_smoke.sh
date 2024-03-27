#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

source smoke_env.sh

APPLICATIONS="dgemm dgemm_tiny nekbone minife amg nasft hpcg hpl_mkl hpl_netlib pennant"

function check {
   if [ $? -ne 0 ]; then
       result=1
   fi
}


function print_result {
    local LOG_NAME="smoke_test_gen_results.log"

    if [ $result -eq 0 ]; then
        echo -e "\e[1;32m[ PASS ] $EXP_TYPE with $APP\e[0m" 1>&2
        echo "${APP} ${EXP_TYPE} PASS" >> ${LOG_NAME}
    elif [ $result -eq 1 ]; then
        echo -e "\e[1;31m[ FAIL ] $EXP_TYPE with $APP\e[0m" 1>&2
        echo "${APP} ${EXP_TYPE} FAIL" >> ${LOG_NAME}
    elif [ $result -eq 2 ]; then
        echo -e "\e[1;33m[ SKIP ] $EXP_TYPE with $APP\e[0m" 1>&2
        echo "${APP} ${EXP_TYPE} SKIP" >> ${LOG_NAME}
    fi
}


function find_output_dirs {
    if [ -z "${SLURM_JOB_ID}" ]; then
        find . -regextype sed -regex "./[0-9]*_${APP}_${EXP_TYPE}" -type d
    else
        find . -regextype sed -regex "./${SLURM_JOB_ID}_${APP}_${EXP_TYPE}" -type d
    fi
}


function gen_all_monitor {

    EXP_TYPE=monitor
    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_achieved_power.py --output-dir=${OUTDIR} --show-details
            check
        done
        if [ -z "${OUTPUT_DIRS}" ]; then
            result=2
        fi
        print_result
    done
}


function gen_all_power_sweep {

    EXP_TYPE=power_sweep
    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_balancer_comparison.py --output-dir=${OUTDIR} --show-details
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_node_efficiency.py --output-dir=${OUTDIR} --show-details
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_power_sweep_summary.py --output-dir=${OUTDIR}
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_balancer_comparison.py --output-dir=${OUTDIR} --show-details
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_balancer_power_limit.py ${OUTDIR}/*power_balancer*.trace-*
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_policy_recommendation.py --path ${OUTDIR}
            check
        done
        if [ -z "${OUTPUT_DIRS}" ]; then
            result=2
        fi
        print_result
    done
}


function gen_all_power_balancer_energy {

    EXP_TYPE=power_balancer_energy
    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/energy_efficiency/gen_plot_profile_comparison.py --output-dir=${OUTDIR} --show-details
            check
        done
        if [ -z "${OUTPUT_DIRS}" ]; then
            result=2
        fi
        print_result
    done
}


function gen_all_freq_sweep {

    EXP_TYPE=frequency_sweep
    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_runtime_energy.py --output-dir=${OUTDIR} --show-details
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_frequency_map.py --output-dir=${OUTDIR} --show-details
            check
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_region_summary.py --output-dir=${OUTDIR} --show-details
            check
        done
        if [ -z "${OUTPUT_DIRS}" ]; then
            result=2
        fi
        print_result
    done
}


function gen_all_uncore_freq_sweep {

    EXP_TYPE=uncore_frequency_sweep
    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_heatmap.py --output-dir=${OUTDIR} --show-details
            check
        done
        if [ -z "${OUTPUT_DIRS}" ]; then
            result=2
        fi
        print_result
    done
}


function gen_all_barrier_frequency_sweep {

    EXP_TYPE=barrier_frequency_sweep
    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/energy_efficiency/gen_plot_profile_comparison.py --output-dir=${OUTDIR} --show-details
            check
        done
        if [ -z "${OUTPUT_DIRS}" ]; then
            result=2
        fi
        print_result
    done
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 monitor|power_sweep|frequency_sweep|uncore_frequency_sweep|barrier_frequency_sweep|power_balancer_energy"
    exit -1
fi

name=$1

if [ "$name" == "monitor" ]; then
    gen_all_monitor
elif [ "$name" == "power_sweep" ]; then
    gen_all_power_sweep
elif [ "$name" == "power_balancer_energy" ]; then
    gen_all_power_balancer_energy
elif [ "$name" == "frequency_sweep" ]; then
    gen_all_freq_sweep
elif [ "$name" == "uncore_frequency_sweep" ]; then
    gen_all_uncore_freq_sweep
elif [ "$name" == "barrier_frequency_sweep" ]; then
    gen_all_barrier_freq_sweep
else
    echo "Error: Unknown name: $name" 1>&2
    exit -1
fi
