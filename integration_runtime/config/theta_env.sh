#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# THETA BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

module unload darshan

export CC='cc -dynamic'
export CXX='CC -dynamic'
export MPICC='cc -dynamic'
export MPICXX='CC -dynamic'
export FC=ftn
export F77=ftn
export F90=ftn
export MPIFORT=ftn
export MPIFC=ftn
export MPIF77=ftn
export MPIF90=ftn

AVX_FLAG='-xCORE-AVX2'
export CFLAGS=${AVX_FLAG}
export CXXFLAGS=${AVX_FLAG}
export FCFLAGS="-dynamic ${AVX_FLAG}"
export FFLAGS="-dynamic ${AVX_FLAG}"

