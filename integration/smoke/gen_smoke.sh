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

source geopm_env.sh
EXP_DIR=$GEOPM_SRC/integration/experiment

function check {
   if [ $? -ne 0 ]; then
       result=1
   fi
}


function print_result {
    if [ $result -ne 0 ]; then
        echo -e "\e[1;31m[ FAIL ] $EXP_TYPE with $APP\e[0m" 1>&2
    else
        echo -e "\e[1;32m[ PASS ] $EXP_TYPE with $APP\e[0m" 1>&2
    fi
}


function find_output_dirs {
    find . -regextype sed -regex "./[0-9]*_${APP}_${EXP_TYPE}" -type d
}


function gen_all_monitor {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife"
    EXP_TYPE=monitor

    for APP in ${APPLICATIONS}; do
        result=0
        OUTPUT_DIRS=$(find_output_dirs)
        for OUTDIR in $OUTPUT_DIRS; do
            python3 ${EXP_DIR}/${EXP_TYPE}/gen_plot_achieved_power.py --output-dir=${OUTDIR} --show-details
            check
        done
        print_result
    done
}


function gen_all_power_sweep {
    APPLICATIONS="dgemm dgemm_tiny nekbone minife"
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
        done
        print_result
    done
}


function gen_all_freq_sweep {

    APPLICATIONS="dgemm dgemm_tiny nekbone minife"
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
        print_result
    done
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 monitor|power_sweep|freq_sweep"
    exit -1
fi

name=$1

if [ "$name" == "monitor" ]; then
    gen_all_monitor
elif [ "$name" == "power_sweep" ]; then
    gen_all_power_sweep
elif [ "$name" == "freq_sweep" ]; then
    gen_all_freq_sweep
else
    echo "Error: Unknown name: $name" 1>&2
    exit -1
fi
