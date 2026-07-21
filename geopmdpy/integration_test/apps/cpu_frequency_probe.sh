#!/bin/bash
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Trivial CPU-bound, frequency-sensitive workload for the geopmopt Phase 0
# regression integration test.  It performs a fixed-size integer busy loop,
# times it, and prints a throughput figure of merit:
#
#     GEOPMOPT-FOM: <iterations-per-second>
#
# The loop is compute-bound and single-threaded, so a higher CPU core
# frequency yields a higher throughput.  A maximizing geopmopt run must
# therefore prefer the highest frequency in the search grid.  geopmopt applies
# the candidate frequency cap live before launching this script, so the
# workload itself performs no control writes.  The iteration count is tunable
# through GEOPMOPT_PROBE_ITERS for slower or faster hosts.  GEOPMOPT_PROBE_COOLDOWN_S
# (default 0) idle-sleeps before the timed loop so consecutive trials start from
# a comparable package temperature; the sleep is outside the timed region and
# does not affect the reported throughput.  GEOPMOPT_PROBE_CPU (default unset)
# pins the loop to a single CPU so scheduler migration between cores does not
# perturb the timing.

set -euo pipefail

iterations="${GEOPMOPT_PROBE_ITERS:-3000000}"
cooldown_s="${GEOPMOPT_PROBE_COOLDOWN_S:-0}"
probe_cpu="${GEOPMOPT_PROBE_CPU:-}"

if [[ -n "${probe_cpu}" ]] && command -v taskset >/dev/null 2>&1; then
    # Restrict this shell (and thus the busy loop below) to one CPU.
    taskset -cp "${probe_cpu}" $$ >/dev/null 2>&1 || true
fi

if [[ "${cooldown_s}" != "0" ]]; then
    sleep "${cooldown_s}"
fi

start_ns=$(date +%s%N)
sum=0
for (( i = 0; i < iterations; i++ )); do
    # Assignment form (not a bare "(( ))" command) so a zero result does not
    # trip "set -e"; integer wraparound is harmless because sum is unused.
    sum=$(( sum + i ))
done
end_ns=$(date +%s%N)

elapsed_ns=$(( end_ns - start_ns ))
if (( elapsed_ns <= 0 )); then
    elapsed_ns=1
fi

# throughput = iterations / seconds = iterations * 1e9 / elapsed_ns
fom=$(awk -v it="${iterations}" -v ns="${elapsed_ns}" \
    'BEGIN { printf "%.3f", it * 1.0e9 / ns }')
echo "GEOPMOPT-FOM: ${fom}"
