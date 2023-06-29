#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# AUSTRALIS BUILD/RUN ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

export CC=icx
export CXX=icpx
export MPICC=mpicc
export MPICXX=mpic++
export FC=ifx
export F77=ifx
export F90=ifx
export MPIFORT=mpifort
export MPIFC=mpifort
export MPIF77=mpifort
export MPIF90=mpifort
export PALS_PMI=pmix
export GEOPM_LAUNCHER=pals
