#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# Exercise the #3927 recoverable timeout-penalty path.  The frequency-scaled
# sleep workload combined with a short --application-timeout makes low-frequency
# trials time out.  With the default --penalty=auto those trials are penalized
# and logged rather than aborting the run, so the optimization completes and
# still emits a control configuration.

set -euo pipefail
set -x

output_config="optimal-frequency.config"
run_log="timeout-run.log"

cleanup() {
    rm -f "${output_config}" "${run_log}"
}
trap cleanup EXIT

geopmopt --verbosity=2 \
         --sweep cpu-freq@board \
         --minimize \
         --metric-regex='GEOPMOPT-FOM: ([0-9.]+)' \
         --application-timeout=2 \
         --trials=8 \
         --n-initial-points=4 \
         --output-file="${output_config}" \
         -- ./check_geopmopt_freq_sleep_run.sh 2>&1 | tee "${run_log}"

# The run completed despite one or more timeouts and produced a configuration.
grep -q "CPU_FREQUENCY_MAX_CONTROL board 0" "${output_config}"
# At least one recoverable timeout was penalized rather than aborting the run.
grep -qi "Penalizing failed trial" "${run_log}"
