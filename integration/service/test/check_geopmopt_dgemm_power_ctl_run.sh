#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

BENCH_CONF=bench.conf
CONTROL_CONFIG=init-control.config
REPORT_OUTPUT=$(mktemp test_geopmopt_dgemm_ctl_report-XXXXXXXX.yaml)
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
OMP_NUM_THREADS=51 \
geopmlaunch pals -n 2 --ppn 2 \
                 --geopm-report=${REPORT_OUTPUT} \
                 --geopm-init-control=${CONTROL_CONFIG} \
                 --geopm-period=1 \
                 --geopm-program-filter=geopmbench \
                 --geopm-affinity-enable \
                 -- geopmbench ${BENCH_CONF}
wait
python3 <<EOF
from yaml import safe_load
from glob import glob

fom = 0
num_hosts = 0
path = glob("$REPORT_OUTPUT*")[0]
with open(path) as fid:
    report = safe_load(fid)
hosts = list(report['Hosts'].keys())
fom += sum([report['Hosts'][hh]['Regions'][0]['runtime (s)'] *
            report['Hosts'][hh]['Regions'][0]['power (W)'] for hh in hosts])
num_hosts += len(hosts)
fom /= num_hosts
print(f'GEOPMOPT-FOM: {fom}')
EOF

# Give geopmctl and geopmd 2 seconds to clean up
sleep 2
