#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

BENCH_CONF=bench.conf
CONTROL_CONFIG=init-control.config
REPORT_OUTPUT=test_geopmopt_dgemm_ctl_report.yaml
trap 'rm -f "${BENCH_CONF}" "{CONTROL_CONFIG}" "${REPORT_OUTPUT}"' EXIT
cat <<EOF > ${BENCH_CONF}
{
  "loop-count": 100,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF
echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" > ${CONTROL_CONFIG}
if [ $# -eq 1 ]; then
    cat $1 >> ${CONTROL_CONFIG}
fi
GEOPM_REPORT=${REPORT_OUTPUT} GEOPM_INIT_CONTROL=${CONTROL_CONFIG} GEOPM_PERIOD=1e-2 geopmctl &
GEOPMCTL_PID=$!
GEOPM_PROGRAM_FILTER=geopmbench geopmbench --verbose ${BENCH_CONF}
wait
python3 <<EOF
from yaml import safe_load
from socket import gethostname
with open(f"$REPORT_OUTPUT-{gethostname()}") as fid:
    report = safe_load(fid)
hosts = list(report['Hosts'].keys())
fom = sum([report['Hosts'][hh]['Epoch Totals']['runtime (s)'] for hh in hosts]) / len(hosts)
print(f'GEOPMOPT-FOM: {fom}')
EOF
