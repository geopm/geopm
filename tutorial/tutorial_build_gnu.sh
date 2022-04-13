#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

. tutorial_env.sh

# OMP_FLAGS: Flags for enabling OpenMP
if [ ! "$OMP_FLAGS" ]; then
    OMP_FLAGS="-fopenmp"
fi

make \
CFLAGS="$GEOPM_CFLAGS $OMP_FLAGS -std=gnu11 -mavx" \
CXXFLAGS="$GEOPM_CFLAGS $OMP_FLAGS -std=gnu++11 -mavx" \
LDFLAGS="$GEOPM_LDFLAGS $OMP_FLAGS -lm -lrt -mavx"
