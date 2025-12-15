#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UTIL_PY="${SCRIPT_DIR}/geopmopt_test_utils.py"

bench_conf=$(mktemp -p "${PWD}" geopmopt_dgemm_ctl_bench.XXXXXX)
control_config=$(mktemp -p "${PWD}" geopmopt_dgemm_ctl_control.XXXXXX)
report_prefix=$(mktemp -p "${PWD}" geopmopt_dgemm_ctl_report.XXXXXX)
rm -f "${report_prefix}"

geopmctl_pid=""

cleanup() {
  if [[ -n "${geopmctl_pid}" ]] && kill -0 "${geopmctl_pid}" 2>/dev/null; then
    kill "${geopmctl_pid}" 2>/dev/null || true
    wait "${geopmctl_pid}" 2>/dev/null || true
  fi
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

GEOPM_REPORT="${report_prefix}" \
GEOPM_INIT_CONTROL="${control_config}" \
GEOPM_PERIOD=1e-2 \
geopmctl &
geopmctl_pid=$!

GEOPM_PROGRAM_FILTER=geopmbench geopmbench --verbose "${bench_conf}"

wait "${geopmctl_pid}"

python3 "${UTIL_PY}" epoch-runtime --report-pattern "${report_prefix}"'*'
