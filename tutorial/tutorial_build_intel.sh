#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

. tutorial_env.sh

# OMP_FLAGS: Flags for enabling OpenMP
if [ ! "$OMP_FLAGS" ]; then
    OMP_FLAGS="-qopenmp"
fi

export CC=${CC:-icx}
export CXX=${CXX:-icpx}
export MPICC=${MPICC:-mpiicc}
export MPICXX=${MPICXX:-mpiicpc}
export MPIFC=${MPIFC:-mpiifort}
export MPIF77=${MPIF77:-mpiifort}

make \
CFLAGS="$GEOPM_CFLAGS $OMP_FLAGS -DTUTORIAL_ENABLE_MKL -D_GNU_SOURCE -std=gnu17 -mavx $CFLAGS" \
CXXFLAGS="$GEOPM_CFLAGS $OMP_FLAGS -DTUTORIAL_ENABLE_MKL -D_GNU_SOURCE -std=c++17 -mavx $CXXFLAGS" \
LDFLAGS="$GEOPM_LDFLAGS $OMP_FLAGS -lm -lrt -qmkl $LDFLAGS"
