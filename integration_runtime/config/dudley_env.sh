#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# DUDLEY BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

export CC=icc
export CXX=icpc
export MPICC=mpicc
export MPICXX=mpic++
export FC=ifort
export F77=ifort
export F90=ifort
export MPIFORT=mpifort
export MPIFC=mpifort
export MPIF77=mpifort
export MPIF90=mpifort
export FI_PROVIDER=psm2
