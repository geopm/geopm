#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

# Set variables for workload
DIRNAME=PENNANT
ARCHIVE=pennant_1_0_1.tgz
URL=https://asc.llnl.gov/sites/asc/files/2020-09/

# Run helper functions
clean_source ${DIRNAME}
get_archive ${ARCHIVE} ${URL}
unpack_archive ${ARCHIVE}
setup_source_git ${DIRNAME}

# Build application
cd ${DIRNAME}
make USEGEOPM=1 EPOCH_TO_OUTERLOOP=100 USEGEOPMMARKUP=1
mv build build_geopm_epoch100
make clean
make USEGEOPM=1 EPOCH_TO_OUTERLOOP=1 USEGEOPMMARKUP=1
mv build build_geopm_epoch1
make clean
make
