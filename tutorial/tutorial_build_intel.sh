#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

. tutorial_env.sh

# OMP_FLAGS: Flags for enabling OpenMP
if [ ! "$OMP_FLAGS" ]; then
    OMP_FLAGS="-qopenmp"
fi

export CC=${CC:-icc}
export CXX=${CXX:-icpc}
export MPICC=${MPICC:-mpiicc}
export MPICXX=${MPICXX:-mpiicpc}
export MPIFC=${MPIFC:-mpiifort}
export MPIF77=${MPIF77:-mpiifort}

make \
CFLAGS="$GEOPM_CFLAGS $OMP_FLAGS -DTUTORIAL_ENABLE_MKL -D_GNU_SOURCE -std=c99 -xAVX $CFLAGS" \
CXXFLAGS="$GEOPM_CFLAGS $OMP_FLAGS -DTUTORIAL_ENABLE_MKL -D_GNU_SOURCE -std=c++11 -xAVX $CXXFLAGS" \
LDFLAGS="$GEOPM_LDFLAGS $OMP_FLAGS -lm -lrt -mkl -xAVX $LDFLAGS"
