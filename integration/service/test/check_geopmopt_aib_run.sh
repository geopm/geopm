#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

BENCH_CONF='-b 0.5'
CONTROL_CONFIG=init-control.config
REPORT_OUTPUT=$(mktemp test_geopmopt_dgemm_ctl_report-XXXXXXXX.yaml)
trap 'rm -f "{CONTROL_CONFIG}" "${REPORT_OUTPUT}"' EXIT
echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" > ${CONTROL_CONFIG}
if [ $# -eq 1 ]; then
    cat $1 >> ${CONTROL_CONFIG}
fi
geopmlaunch pals -n 2 --ppn 2 \
                 --geopm-report=${REPORT_OUTPUT} \
                 --geopm-init-control=${CONTROL_CONFIG} \
                 --geopm-period=1 \
                 --geopm-program-filter=geopmbench \
                 --geopm-affinity-enable \
                 -- bench_avx512 ${BENCH_CONF}
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










#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -ex

BENCH_CONF='-b 0.5'
CONTROL_CONFIG=init-control.config
REPORT_OUTPUT=test_geopmopt_aib_report.yaml
trap 'rm -f "{CONTROL_CONFIG}" "${REPORT_OUTPUT}"' EXIT
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
