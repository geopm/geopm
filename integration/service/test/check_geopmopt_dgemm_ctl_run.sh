#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

BENCH_CONF=bench.conf
trap 'rm -f "${BENCH_CONF}"' EXIT
cat <<EOF > $BENCH_CONF
{
  "loop-count": 10,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF
echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" > init-control.config
if [ $# -eq 2 ]; then
    cat $1 >> init-control.config
fi
GEOPM_REPORT=report.yml \
GEOPM_INIT_CONTROL=init-control.config \
    geopmctl &
GEOPMCTL_PID=$!
GEOPM_PROGRAM_FILTER=geopmbench \
    geopmbench --verbose ${BENCH_CONF}
kill -15 ${GEOPMCTL_PID}
tail -f /dev/null --pid ${GEOPMCTL_PID}
python3 <<EOF
from yaml import safe_load
with open('report.yaml') as fid:
    report = safe_load(fid)
hosts = list(report['Hosts'].keys())
fom = sum([report['Hosts'][hh]['Epoch Totals']['runtime (s)'] for hh in hosts]) / len(hosts)
print(f'GEOPMOPT-FOM: {fom}')
EOF
