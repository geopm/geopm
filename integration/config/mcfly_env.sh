#!/bin/bash
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# MCFLY BUILD/RUN ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

export CC=icx
export CXX=icpx
export MPICC=mpiicc
export MPICXX=mpiicpc
export FC=ifx
export F77=ifx
export F90=ifx
export MPIFORT=mpiifort
export MPIFC=mpiifort
export MPIF77=mpiifort
export MPIF90=mpiifort
export GEOPM_LAUNCHER=srun
