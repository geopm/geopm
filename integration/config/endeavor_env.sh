#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# ENDEAVOR BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.


if [ "${SETVARS_COMPLETED:-}" = "1" ] ; then
    echo "INFO: One API setvars.sh is already sourced. Skipped sourcing again."
else
    source /opt/intel/oneAPI/latest/setvars.sh
fi

export GEOPM_LAUNCHER=impi
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
