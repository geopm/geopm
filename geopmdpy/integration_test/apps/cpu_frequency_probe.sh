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
# through GEOPMOPT_PROBE_ITERS for slower or faster hosts.

set -euo pipefail

iterations="${GEOPMOPT_PROBE_ITERS:-3000000}"

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
