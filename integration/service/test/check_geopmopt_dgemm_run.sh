#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UTIL_PY="${SCRIPT_DIR}/geopmopt_test_utils.py"

bench_conf=$(mktemp -p "${PWD}" geopmopt_dgemm_bench.XXXXXX)
control_config=$(mktemp -p "${PWD}" geopmopt_dgemm_control.XXXXXX)
report_prefix=$(mktemp -p "${PWD}" geopmopt_dgemm_report.XXXXXX)
rm -f "${report_prefix}"

cleanup() {
  rm -f "${bench_conf}" "${control_config}" "${report_prefix}" "${report_prefix}"-*
}
trap cleanup EXIT

cat <<EOF >"${bench_conf}"
{
  "loop-count": 10,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF

echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" >"${control_config}"
if [[ $# -ge 1 ]]; then
  cat "$1" >>"${control_config}"
fi

geopmlaunch pals -n 4 --ppn 2 \
  --geopm-program-filter=geopmbench \
  --geopm-report="${report_prefix}" \
  --geopm-init-control="${control_config}" \
  -- geopmbench --verbose "${bench_conf}"

python3 "${UTIL_PY}" epoch-runtime --report-pattern "${report_prefix}"'*'
