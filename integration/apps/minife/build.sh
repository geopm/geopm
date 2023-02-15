#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

# Set variables for workload
DIRNAME=miniFE_openmp-2.0-rc3
ARCHIVE=${DIRNAME}.zip
URL=https://asc.llnl.gov/sites/asc/files/2020-09

# Run helper functions
clean_source ${DIRNAME}
rm -rf __MACOSX
get_archive ${ARCHIVE} ${URL}
unpack_archive ${ARCHIVE}
rm -rf __MACOSX
setup_source_git ${DIRNAME}

# Build application
cd ${DIRNAME}/src
make -f Makefile.intel.openmp
