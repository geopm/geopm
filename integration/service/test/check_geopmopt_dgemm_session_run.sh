#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UTIL_PY="${SCRIPT_DIR}/geopmopt_test_utils.py"

bench_conf=$(mktemp -p "${PWD}" geopmopt_dgemm_session_bench.XXXXXX)
control_config=$(mktemp -p "${PWD}" geopmopt_dgemm_session_control.XXXXXX)
signal_config=$(mktemp -p "${PWD}" geopmopt_dgemm_session_signal.XXXXXX)
report_file=$(mktemp -p "${PWD}" geopmopt_dgemm_session_report.XXXXXX)
daemon_pid_file=$(mktemp -p "${PWD}" geopmopt_dgemm_session_daemon.XXXXXX.pid)
rm -f "${report_file}"

daemon_pid=""

cleanup() {
  local pid="${daemon_pid}"
  if [[ -z "${pid}" && -f "${daemon_pid_file}" ]]; then
    pid=$(<"${daemon_pid_file}")
  fi
  if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
    kill "${pid}" 2>/dev/null || true
    tail --pid="${pid}" -f /dev/null || true
  fi
  rm -f "${bench_conf}" "${control_config}" "${signal_config}" "${report_file}" "${daemon_pid_file}"
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

echo "TIME board 0" >"${signal_config}"
echo "CPU_ENERGY board 0" >>"${signal_config}"

geopmsession --daemon "${daemon_pid_file}" \
  --period 1e-2 \
  --report-out "${report_file}" \
  --trace-out /dev/null \
  --signal-config "${signal_config}" \
  --control-config "${control_config}"

geopmbench --verbose "${bench_conf}"

daemon_pid=$(<"${daemon_pid_file}")
if kill -0 "${daemon_pid}" 2>/dev/null; then
  kill "${daemon_pid}" 2>/dev/null || true
  tail --pid="${daemon_pid}" -f /dev/null || true
fi
rm -f "${daemon_pid_file}"

python3 "${UTIL_PY}" session-duration --report-pattern "${report_file}"
