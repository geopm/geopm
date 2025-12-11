#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

if [ -z "${AIB_INTENSITY}" ]; then
    AIB_INTENSITY=2
fi

AIB_ITERATIONS=200
if [ ${AIB_INTENSITY} -eq 0 ]; then
    AIB_ITERATIONS=476
elif [ ${AIB_INTENSITY} -eq 1 ]; then
    AIB_ITERATIONS=469
elif [ ${AIB_INTENSITY} -eq 2 ]; then
    AIB_ITERATIONS=452
elif [ ${AIB_INTENSITY} -eq 4 ]; then
    AIB_ITERATIONS=420
elif [ ${AIB_INTENSITY} -eq 8 ]; then
    AIB_ITERATIONS=355
elif [ ${AIB_INTENSITY} -eq 16 ]; then
    AIB_ITERATIONS=280
elif [ ${AIB_INTENSITY} -eq 32 ]; then
    AIB_ITERATIONS=173
fi

BENCH_CONF="-i ${AIB_ITERATIONS} -b ${AIB_INTENSITY}"
CONTROL_CONFIG=init-control.config
REPORT_OUTPUT=$(mktemp test_geopmopt_aib_report-XXXXXXXX.yaml)
trap 'rm -f "{CONTROL_CONFIG}" "${REPORT_OUTPUT}*"' EXIT
echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" > ${CONTROL_CONFIG}
if [ $# -eq 1 ]; then
    cat $1 >> ${CONTROL_CONFIG}
fi
geopmlaunch pals -n 100 --ppn 100 \
                 --geopm-report=${REPORT_OUTPUT} \
                 --geopm-init-control=${CONTROL_CONFIG} \
                 --geopm-period=1 \
                 --geopm-program-filter=bench_avx512 \
                 --geopm-affinity-enable \
                 -- bench_avx512 ${BENCH_CONF}
wait
sleep 2
python3 <<EOF
from yaml import safe_load
from glob import glob

fom = 0
num_hosts = 0
path = glob("$REPORT_OUTPUT-*")[0]
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

