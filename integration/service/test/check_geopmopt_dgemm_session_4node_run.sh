#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

BENCH_CONF=bench.conf # Shared file
CONTROL_CONFIG=init-control.config # Shared file
DAEMON_PID_FILE=/tmp/test_geopm_dgemm_session_4node_daemon.pid # Node local file
SIGNAL_CONFIG=test_geopm_dgemm_session_4node_signal.conf # Shared file
REPORT_OUTPUT=test_geopm_dgemm_session_4node_report.yaml # Shared file with hostname appended
MPI_EXEC='mpiexec'
MPI_ARGS='-ppn 1 -n 4 --'
REMOTE_TRAP='test -e "${DAEMON_PID_FILE}" && kill $(cat "{DAEMON_PID_FILE}") >&/dev/null || true; rm -f "${DAEMON_PID_FILE}"'
#trap '"${MPI_EXEC}" bash -c "${REMOTE_TRAP}"; rm -f "${BENCH_CONF}" "${SIGNAL_CONFIG}" "${CONTROL_CONFIG}"' EXIT
cat <<EOF > ${BENCH_CONF}
{
  "loop-count": 10,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF
echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" > ${CONTROL_CONFIG}
if [ $# -eq 1 ]; then
    cat $1 >> ${CONTROL_CONFIG}
fi
echo "TIME board 0" > ${SIGNAL_CONFIG}
echo "CPU_ENERGY board 0" >> ${SIGNAL_CONFIG}
${MPI_EXEC} ${MPI_ARGS} \
geopmsession --daemon ${DAEMON_PID_FILE} \
             --period 1e-2 \
             --report-out ${REPORT_OUTPUT} \
             --trace-out /dev/null \
             --signal-config ${SIGNAL_CONFIG} \
             --control-config ${CONTROL_CONFIG} \
             --append-hostname

${MPI_EXEC} ${MPI_ARGS} \
geopmbench --verbose ${BENCH_CONF}

${MPI_EXEC} ${MPI_ARGS} \
bash -c 'DAEMON_PID=$(cat ${DAEMON_PID_FILE}); kill ${DAEMON_PID}; tail -f /dev/null --pid ${DAEMON_PID}; rm -f ${DAEMON_PID_FILE}'

python3 <<EOF
from yaml import safe_load
from glob import glob
total = 0.0
host_count = 0
for rf in glob("$REPORT-OUTPUT" + "-*'):
    with open(rf) as fid:
        rpt = safe_load(fid)
        total += rpt['metrics']['TIME']['last'] - rpt['metrics']['TIME']['first']
        host_count += 1
fom = total / host_count
print(f'GEOPMOPT-FOM: {fom}')
EOF
