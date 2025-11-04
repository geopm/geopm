#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# This script is sourced from platform_sweeps.sh


# Sweep variables setup

if [[ "${SWEEP_TYPE}" == "CPU" ]]; then
    PROGRAM_NAME="bench_avx512"
    BINARY_PATH_PLUS_FLAGS="${GEOPM_SOURCE}/integration/apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx512 -i 10 -b 1 16"
    BOARD_POWER_STEP=100
    BOARD_MIN_POWER=1600
    BOARD_MAX_POWER=2700
elif [[ "${SWEEP_TYPE}" == "GPU" ]]; then
    PROGRAM_NAME="nstream-onemkl"
    BINARY_PATH_PLUS_FLAGS="${GEOPM_SOURCE}/integration/apps/parres/Kernels/Cxx11/nstream-onemkl 75 3000000000"
    BOARD_POWER_STEP=100
    BOARD_MIN_POWER=2000
    BOARD_MAX_POWER=2600
else
    echo "Error: Incorrect sweep type in first parameter"
    exit 1
fi


TRIAL_COUNT=3 # characterization time increases linearly with this parameter
CORE_SKIP_LIST="0,1,52,53" # e.g., skip first 2 cores on each 52-core socket on a dual-socket system

# Aurora environment setup
module load geopm-runtime
module load py-geopmpy
export PYTHONPATH=${GEOPM_SOURCE}:${GEOPM_SOURCE}/geopmdpy:${HOME}/.local/lib/python3.6/site-packages:$PYTHONPATH
