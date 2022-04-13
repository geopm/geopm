#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# SMNG BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

# Default Intel Toolchain compiler overrides
# The MPI compiler wrappers are supported by Intel, but are not Intel specific.
module swap intel intel/19.1
module load python/3.6_intel
unset OMP_NUM_THREADS # Set to 1 by a default lmod module

# One time setup:
# python -m pip install --user -U pip
# python -m pip install --user --ignore-installed -r geopm/scripts/requirements.txt

export MPIFORT=mpiifort
export MPIFC=mpiifort
export MPIF77=mpiifort
export MPIF90=mpiifort

export GEOPM_USER_ACCOUNT=h1906
export GEOPM_SYSTEM_DEFAULT_QUEUE=test
export GEOPM_SBATCH_EXTRA_LINES="
#SBATCH --constraint=\"noscratch\"
#SBATCH --ear off
${GEOPM_SBATCH_EXTRA_LINES}
"

# Notes
##SBATCH --export=NONE # This breaks the env of srun calls within SBATCH scripts

