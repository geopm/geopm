#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -e
set -x
SCRIPT_DIR=$(dirname $(realpath $0))
REPORT_FILE=$(mktemp)
PID_FILE=$(mktemp)
echo "TIME board 0" > session.conf
geopmsession --daemon ${PID_FILE} -r ${REPORT_FILE} -o /dev/null -i session.conf -p 1e-4 -t 100000
sleep 10
kill $(cat ${PID_FILE})
wait
python3 -c 'from yaml import safe_load; fid=open("'${REPORT_FILE}'"); rr=safe_load(fid); assert(abs(rr["sample-time-total"] - 10) < 0.1)'
rm -f ${REPORT_FILE}
rm -f ${PID_FILE}
