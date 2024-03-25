#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

# Set variables for workload
DIRNAME=hpl-2.3
ARCHIVE=${DIRNAME}.tar.gz
URL=https://www.netlib.org/benchmark/hpl/

# Run helper functions
clean_source ${DIRNAME}
get_archive ${ARCHIVE} ${URL}
unpack_archive ${ARCHIVE}
setup_source_git ${DIRNAME}

# Build application
cd ${DIRNAME}
make arch=Linux_Intel64
