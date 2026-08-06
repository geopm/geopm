#!/bin/bash
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# Exercise the #4043 bounded-energy objective: minimize energy subject to a
# figure-of-merit constraint.  The figure of merit here is the dgemm wall-clock
# time (lower is better, so --minimize), and the bound is set generously so the
# feasible region is non-empty on any hardware.  The run must complete, report
# that the bound was satisfied, and emit a valid control configuration.

set -euo pipefail
set -x

output_config="optimal-frequency.config"
summary_log="bounded-summary.log"

cleanup() {
    rm -f "${output_config}" "${summary_log}"
}
trap cleanup EXIT

STICKER_FREQ=$(geopmread CPU_FREQUENCY_STICKER board 0)
MAX_FREQ=$((STICKER_FREQ - 300000000))

# A one-hour figure-of-merit bound is always satisfiable, so the optimizer
# minimizes energy over the feasible region rather than reporting a violation.
geopmopt --verbosity=2 \
         --sweep cpu-freq@board=:${MAX_FREQ} \
         --efficiency=cpu \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --metric-bound=3600 \
         --minimize \
         --output-file="${output_config}" \
         --trials=20 \
         -- ./check_geopmopt_dgemm_bench_run.sh | tee "${summary_log}"

grep -q "CPU_FREQUENCY_MAX_CONTROL board 0" "${output_config}"
grep -q "Constraints were satisfied" "${summary_log}"
