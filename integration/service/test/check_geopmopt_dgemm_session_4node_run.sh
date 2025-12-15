#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UTIL_PY="${SCRIPT_DIR}/geopmopt_test_utils.py"

bench_conf=$(mktemp -p "${PWD}" geopmopt_dgemm_session4_bench.XXXXXX)
control_config=$(mktemp -p "${PWD}" geopmopt_dgemm_session4_control.XXXXXX)
signal_config=$(mktemp -p "${PWD}" geopmopt_dgemm_session4_signal.XXXXXX)
report_prefix=$(mktemp -p "${PWD}" geopmopt_dgemm_session4_report.XXXXXX)
daemon_pid_file=$(mktemp -p "${PWD}" geopmopt_dgemm_session4_daemon.XXXXXX.pid)
rm -f "${report_prefix}" "${daemon_pid_file}"

MPI_EXEC=${MPI_EXEC:-mpiexec}
MPI_ARGS=(-ppn 1 -n 4 --)

mpi_run() {
  "${MPI_EXEC}" "${MPI_ARGS[@]}" "$@"
}

remote_cleanup_daemon() {
  mpi_run bash -c 'if [ -f "'"${daemon_pid_file}"'" ]; then \
    pid=$(cat "'"${daemon_pid_file}"'"); \
    if kill -0 "${pid}" 2>/dev/null; then \
      kill "${pid}" 2>/dev/null || true; \
      tail --pid="${pid}" -f /dev/null || true; \
    fi; \
    rm -f "'"${daemon_pid_file}"'"; \
  fi' || true
}

cleanup() {
  remote_cleanup_daemon
  rm -f "${bench_conf}" "${control_config}" "${signal_config}" "${report_prefix}" "${report_prefix}"-*
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

mpi_run geopmsession \
  --daemon "${daemon_pid_file}" \
  --period 1e-2 \
  --report-out "${report_prefix}" \
  --trace-out /dev/null \
  --signal-config "${signal_config}" \
  --control-config "${control_config}" \
  --append-hostname

mpi_run geopmbench --verbose "${bench_conf}"

remote_cleanup_daemon

python3 "${UTIL_PY}" session-duration --report-pattern "${report_prefix}"'*'
