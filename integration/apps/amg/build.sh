#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

DIRNAME=AMG
GITREPO=https://github.com/LLNL/AMG.git
TOPHASH=3ada8a1
ARCHIVE=${DIRNAME}_${TOPHASH}.tgz

clean_source ${DIRNAME}
get_archive ${ARCHIVE}
if [ -f ${ARCHIVE} ]; then
    unpack_archive ${ARCHIVE}
else
    clone_repo_git ${GITREPO} ${DIRNAME} ${TOPHASH}
fi
setup_source_git ${DIRNAME}

# build
cd ${DIRNAME}
make
