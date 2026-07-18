#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# Plain dgemm workload for the geopmopt runtime, energy, and bounded-energy
# objectives.  geopmopt applies the control settings itself (direct write) and,
# for the energy objectives, wraps this script in geopmsession to sample energy,
# so this workload only needs to run the benchmark and print a wall-clock figure
# of merit.  Any argument (for example a --defer-write config path) is ignored.

set -euo pipefail
set -x

bench_conf=$(mktemp -p "${PWD}" geopmopt_dgemm_bench_wl.XXXXXX)

cleanup() {
  rm -f "${bench_conf}"
}
trap cleanup EXIT

cat <<EOF >"${bench_conf}"
{
  "loop-count": 10,
  "region": ["dgemm"],
  "big-o": [0.2]
}
EOF

start=$(date +%s.%N)
geopmbench --verbose "${bench_conf}"
end=$(date +%s.%N)

python3 -c "import sys; print(f'GEOPMOPT-FOM: {float(sys.argv[2]) - float(sys.argv[1]):.4f}')" "${start}" "${end}"
