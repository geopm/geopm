#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

# Set variables for workload
DIRNAME=NPB3.4.2
ARCHIVE=NPB3.4.2.tar.gz
URL=https://www.nas.nasa.gov/assets/npb

# Run helper functions
clean_source ${DIRNAME}
get_archive ${ARCHIVE} ${URL}
unpack_archive ${ARCHIVE}
setup_source_git ${DIRNAME}

# Build application
cd ${DIRNAME}/NPB3.4-MPI
make suite
