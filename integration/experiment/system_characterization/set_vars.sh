#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# This script is sourced from platform_sweeps.sh


# Sweep variables setup

if [ -z "${1}" ]; then
    echo "Error: No argument supplied for parameter 1. Enter either CPU or GPU"
    exit 1
elif [[ "${1}" == "CPU" ]]; then
    SWEEP_TYPE="CPU"  # can be either "CPU" or "GPU"
    PROGRAM_NAME="bench_avx512"
    BINARY_PATH_PLUS_FLAGS="${GEOPM_SOURCE}/integration/apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx512 -i 10 -b 1 16"
elif [[ "${1}" == "GPU" ]]; then
    SWEEP_TYPE="GPU"  # can be either "CPU" or "GPU"
    PROGRAM_NAME="nstream-onemkl"
    BINARY_PATH_PLUS_FLAGS="${GEOPM_SOURCE}/integration/apps/parres/Kernels/Cxx11/nstream-onemkl 75 3000000000"
else
    echo "Error: Incorrect sweep type in first parameter"
    exit 1
fi


TRIAL_COUNT=3 # characterization time increases linearly with this parameter
CORE_SKIP_LIST="0,1,52,53" # e.g., skip first 2 cores on each 52-core socket on a dual-socket system


# Platform Power Cap specific variables
BOARD_POWER_STEP=100
BOARD_MIN_POWER=2000    # lower the value, lower the power headroom for the components
BOARD_MAX_POWER=2600    # higher the value, more the power headroom for the components


# Aurora environment setup
module load geopm-runtime
module load py-geopmpy
export PYTHONPATH=${GEOPM_SOURCE}:${GEOPM_SOURCE}/geopmdpy:${HOME}/.local/lib/python3.6/site-packages:$PYTHONPATH
