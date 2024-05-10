#!/bin/bash
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# SMNG BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

# Default Intel Toolchain compiler overrides
# The MPI compiler wrappers are supported by Intel, but are not Intel specific.
unset OMP_NUM_THREADS # Set to 1 by a default lmod module

# One time setup:
# python -m pip install --user -U pip
# python -m pip install --user --ignore-installed -r geopm/scripts/requirements.txt
# python -m pip install --user --ignore-installed -r geopm/service/requirements.txt

export CC=icc
export CXX=icpc
export FC=ifort
export F77=ifort
export F90=ifort
export MPICC=mpiicc
export MPICXX=mpiicpc
export MPIFORT=mpiifort
export MPIFC=mpiifort
export MPIF77=mpiifort
export MPIF90=mpiifort

export CPATH=${MKL_INCDIR}:${MKL_F90_INC}:${CPATH} # So that mkl.h can be found

# export GEOPM_USER_ACCOUNT=h1906
export GEOPM_SYSTEM_DEFAULT_QUEUE=test
export GEOPM_SBATCH_EXTRA_LINES="
#SBATCH --constraint=\"noscratch\"
#SBATCH --ear off
${GEOPM_SBATCH_EXTRA_LINES}
"

# Notes
##SBATCH --export=NONE # This breaks the env of srun calls within SBATCH scripts

