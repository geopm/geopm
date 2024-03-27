#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

get_nekbone_source() {
    local DIRNAME=${1}
    # Upstream = https://repocafe.cels.anl.gov/repos/nekbone/trunk/nekbone
    svn checkout file://${GEOPM_APPS_SOURCES}/nekbone-mirror/trunk/nekbone ${DIRNAME}
    cd ${DIRNAME}
    svn log > svn.log
    rm -fr .svn
    cd -
}

# Set variables for workload
DIRNAME=nekbone
clean_source ${DIRNAME}
# Acquire the source:
get_nekbone_source ${DIRNAME}
setup_source_git ${DIRNAME}

# Build
cd ${DIRNAME}/test/example1
GEOPM_BARRIER=yes ./makenek-intel
mv nekbone nekbone-barrier
make clean
./makenek-intel
