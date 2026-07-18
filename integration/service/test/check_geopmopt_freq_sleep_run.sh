#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# Workload for the geopmopt recoverable timeout-penalty test.  It sleeps for a
# time inversely proportional to the applied CPU max-frequency control, so that
# low frequencies deterministically exceed a short --application-timeout (which
# exercises the recoverable-failure penalty path) while high frequencies
# complete.  A figure of merit equal to the sleep time is printed so the run can
# also be driven with --metric-regex.  Any argument is ignored.

set -euo pipefail
set -x

freq=$(geopmread CPU_FREQUENCY_MAX_CONTROL board 0)
sleep_s=$(python3 -c "import sys; print(f'{4.0e9 / float(sys.argv[1]):.3f}')" "${freq}")

sleep "${sleep_s}"

echo "GEOPMOPT-FOM: ${sleep_s}"
