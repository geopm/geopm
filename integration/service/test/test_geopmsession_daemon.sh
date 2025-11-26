#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -e
set -x
TOLERANCE=0.1
SLEEP_TIME=10
SCRIPT_DIR=$(dirname $(realpath $0))
REPORT_FILE=$(mktemp)
PID_FILE=$(mktemp)
trap 'rm -f "${REPORT_FILE}" "${PID_FILE}"' EXIT
echo "TIME board 0" > session.conf
geopmsession --append-hostname --daemon ${PID_FILE} -r ${REPORT_FILE} -o /dev/null -i session.conf -p 1e-4
SERVER_PID=$(cat ${PID_FILE})
sleep ${SLEEP_TIME}
kill ${SERVER_PID}
while ps ${SERVER_PID} >& /dev/null; do sleep 1; done
rm -f ${PID_FILE}
python3 -c 'from yaml import safe_load; fid=open("'${REPORT_FILE}-$(hostname)'"); rr=safe_load(fid); assert(abs(rr["sample-time-total"] - '${SLEEP_TIME}') < '${TOLERANCE}')'
rm -f ${REPORT_FILE}-$(hostname)
