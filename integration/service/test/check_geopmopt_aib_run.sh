#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -euo pipefail
set -x

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UTIL_PY="${SCRIPT_DIR}/geopmopt_test_utils.py"

AIB_INTENSITY=${AIB_INTENSITY:-2}

case "${AIB_INTENSITY}" in
    0)   aib_iterations=476 ;;
    1)   aib_iterations=469 ;;
    2)   aib_iterations=452 ;;
    4)   aib_iterations=420 ;;
    8)   aib_iterations=355 ;;
    16)  aib_iterations=280 ;;
    32)  aib_iterations=173 ;;
    *)
        echo "Unsupported AIB intensity: ${AIB_INTENSITY}" >&2
        exit 1
        ;;
esac

bench_args=("-i" "${aib_iterations}" "-b" "${AIB_INTENSITY}")
control_config=$(mktemp -p "${PWD}" geopmopt_aib_control.XXXXXX)
report_prefix=$(mktemp -p "${PWD}" geopmopt_aib_report.XXXXXX)
rm -f "${report_prefix}"

cleanup() {
    rm -f "${control_config}" "${report_prefix}" "${report_prefix}"-*
}
trap cleanup EXIT

echo "CPU_FREQUENCY_GOVERNOR_CONTROL board 0 0" >"${control_config}"
if [[ $# -ge 1 ]]; then
    cat "$1" >>"${control_config}"
fi

geopmlaunch pals -n 100 --ppn 100 \
    --geopm-report="${report_prefix}" \
    --geopm-init-control="${control_config}" \
    --geopm-period=1 \
    --geopm-program-filter=bench_avx512 \
    --geopm-affinity-enable \
    -- bench_avx512 "${bench_args[@]}"

sleep 2
python3 "${UTIL_PY}" region-energy --report-pattern "${report_prefix}"'*'

# Give geopmctl and geopmd 2 seconds to clean up
sleep 2

