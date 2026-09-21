#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -e
set -x

NNODE=4
TMP_REPORT_FILE=$(mktemp)
TMP_TRACE_FILE=$(mktemp)
SCRIPT_DIR=$(dirname $(realpath $0))

printf "TIME board 0\nCPU_POWER board 0\nCPU_FREQUENCY_STATUS board 0" | mpiexec -n ${NNODE} -ppn 1 geopmsession -t 5 -p 0.005 -r ${TMP_REPORT_FILE} -o ${TMP_TRACE_FILE} --enable-mpi

test -s $TMP_REPORT_FILE
#Check that report is valid yaml and that we got one report per node
python3 ${SCRIPT_DIR}/check_geopmsession_report_valid.py --report ${TMP_REPORT_FILE}\
        --num-nodes ${NNODE}
#Check that we got one report per node, separated by "---"
#[ $((1+`grep "^---$" ${TMP_REPORT_FILE} | wc -l`)) == $NNODE ]

test -s $TMP_TRACE_FILE

rm -f $TMP_REPORT_FILE
rm -f $TMP_TRACE_FILE
