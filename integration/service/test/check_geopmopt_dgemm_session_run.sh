#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex


BENCH_CONF=bench.conf # Shared file
CONTROL_CONFIG=init-control.config # Shared file
DAEMON_PID_FILE=/tmp/test_geopm_dgemm_session_daemon.pid # Node local file
SIGNAL_CONFIG=test_geopm_dgemm_session_signal.conf # Shared file
REPORT_OUTPUT=test_geopm_dgemm_session_report.yaml # Shared file with hostname appended
trap 'rm -f "${BENCH_CONF}" "${SIGNAL_CONFIG}" "${CONTROL_CONFIG}"; mpiexec -n 2 -ppn 1 -- rm -f "${DAEMON_PID_FILE}"' EXIT

cat <<EOF > $BENCH_CONF
{
  "loop-count": 10,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF
echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" > ${CONTROL_CONFIG}
if [ $# -eq 2 ]; then
    cat $1 >> ${CONTROL_CONFIG}
fi
echo "TIME board 0" > ${SIGNAL_CONFIG}
echo "CPU_ENERGY board 0" >> ${SIGNAL_CONFIG}
mpiexec -n 2 -ppn 1 bash -c 'geopmwrite -f '${CONTROL_CONFIG}'; sleep infinity' &
mpiexec -n 2 -ppn 1
        -- geopmsession --daemon ${DAEMON_PID_FILE} \
                        --period 1e-2 \
                        --report-out ${REPORT_OUTPUT} \
                        --trace-out /dev/null \
                        --signal-config ${SIGNAL_CONFIG} \
                        --append-hostname

mpiexec -n 4 -ppn 2 \
	-- geopmbench --verbose ${BENCH_CONF}

mpiexec -n 2 -ppn 1 \
	-- bash -c 'kill $(cat '${DAEMON_PID_FILE}'); pkill sleep; while ps $(cat '${DAEMON_PID_FILE}'); do sleep 1; done; rm -f ${DAEMON_PID_FILE}'

for rr in ${REPORT_OUTPUT}-*.yaml; do
python3 <<EOF
from yaml import safe_load
from glob import glob
energy_total = 0.0
host_count = 0
for rr in glob('test_geopm_dgemm_session_report.yaml-*'):
    with open(rr) as fid:
        rpt = safe_load(fid)
        energy_total += rpt['metrics']['CPU_ENERGY']['last'] -
                        rpt['metrics']['CPU_ENERGY']['first']
        host_count += 1
fom = enrgy_total / host_count
print(f'GEOPMOPT-FOM: {fom}')
EOF
