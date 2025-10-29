#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

# This script is sourced from sys_charac_platform_sweeps.sh

# Sweep variables setup
SWEEP_TYPE="CPU"  # can be either "CPU" or "GPU"
PROGRAM_NAME="binary_name"
BINARY_PATH_PLUS_FLAGS="${BINARY_PATH_PLUS_FLAGS?- Set path to binary including input arguments (if any)}"
export EMPTY_SWEEP_OUTPUT_DIR="${EMPTY_SWEEP_OUTPUT_DIR?- Set variable to an empty directory accessible from the compute node being characterizes}"


TRIAL_COUNT=3 # characterization time increases linearly with this parameter
CORE_SKIP_LIST="0,1,52,53" # e.g., skip first 2 cores on each 52-core socket on a dual-socket system


# Platform Power Cap specific variables
PLATFORM_POWER_ENABLE=1 # set 0 for running frequency sweeps without platform power caps
BOARD_POWER_STEP=100
BOARD_MIN_POWER=2000    # lower the value, lower the power headroom for the components
BOARD_MAX_POWER=2600    # higher the value, more the power headroom for the components


# Aurora environment setup
module load geopm-runtime
module load py-geopmpy
export PYTHONPATH=${GEOPM_SOURCE}:${GEOPM_SOURCE}/geopmdpy:${HOME}/.local/lib/python3.6/site-packages:$PYTHONPATH
