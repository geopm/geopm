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
if [ $# -eq 1 ]; then
    cat $1 >> init-control.config
fi
geopmlaunch pals -n 4 -ppn 2 \
                 --geopm-program-filter=geopmbench \
                 --geopm-report=report.yaml \
                 --geopm-init-control=init-control.config \
                 -- geopmbench --verbose ${BENCH_CONF}

python3 <<EOF
from yaml import safe_load
with open('report.yaml') as fid:
    report = safe_load(fid)
hosts = list(report['Hosts'].keys())
fom = sum([report['Hosts'][hh]['Epoch Totals']['runtime (s)'] for hh in hosts]) / len(hosts)
print(f'GEOPMOPT-FOM: {fom}')
EOF
