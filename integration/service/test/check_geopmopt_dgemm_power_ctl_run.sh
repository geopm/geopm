#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UTIL_PY="${SCRIPT_DIR}/geopmopt_test_utils.py"

bench_conf=$(mktemp -p "${PWD}" geopmopt_dgemm_power_bench.XXXXXX)
control_config=$(mktemp -p "${PWD}" geopmopt_dgemm_power_control.XXXXXX)
report_prefix=$(mktemp -p "${PWD}" geopmopt_dgemm_power_report.XXXXXX)
rm -f "${report_prefix}"

cleanup() {
  rm -f "${bench_conf}" "${control_config}" "${report_prefix}" "${report_prefix}"-*
}
trap cleanup EXIT

cat <<EOF >"${bench_conf}"
{
  "loop-count": 100,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF

echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" >"${control_config}"
if [[ $# -ge 1 ]]; then
  cat "$1" >>"${control_config}"
fi

OMP_NUM_THREADS=51 \
geopmlaunch pals -n 2 --ppn 2 \
  --geopm-report="${report_prefix}" \
  --geopm-init-control="${control_config}" \
  --geopm-period=1 \
  --geopm-program-filter=geopmbench \
  --geopm-affinity-enable \
  -- geopmbench "${bench_conf}"

python3 "${UTIL_PY}" region-energy --report-pattern "${report_prefix}"'*'

# Give geopmctl and geopmd 2 seconds to clean up
sleep 2
