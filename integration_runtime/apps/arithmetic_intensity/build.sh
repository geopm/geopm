#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
set -e

# Get helper functions
source ../build_func.sh

if [[ $# -eq 0 ]]; then
    echo 'Building from a1761f9'
    TOPHASH=a1761f9
elif [[ $# -eq 1 ]]; then
    echo 'Building from head of main'
    TOPHASH=$1
else
    1>&2 echo "usage: $0 [HASH_FOR_HEAD_OF_MAIN]"
    exit 1
fi

if [ ! -x "$(command -v nasm)" ]; then
    DIRNAME=nasm-2.15.05
    ARCHIVE=${DIRNAME}.tar.gz
    URL=https://github.com/netwide-assembler/nasm/archive/refs/tags/

    # Run helper functions
    clean_source ${DIRNAME}
    rm -rf __MACOSX
    get_archive ${ARCHIVE} ${URL}
    unpack_archive ${ARCHIVE}
    # Move to the actual dirname defined above.
    mv $(tar -tzf ${ARCHIVE} | head -1) ${DIRNAME}
    rm -rf __MACOSX
    setup_source_git ${DIRNAME}

    if ! ls ${DIRNAME}/nasm 2> /dev/null; then
        cd ${DIRNAME}
        sh autogen.sh
        sh configure
        make
        cd -
    fi
    export PATH=${PWD}/${DIRNAME}:${PATH}
fi

DIRNAME=ARITHMETIC_INTENSITY
GITREPO=https://github.com/dannosliwcd/arithmetic-intensity
clean_source ${DIRNAME}

ARCHIVE=${DIRNAME}_${TOPHASH}.tgz
get_archive ${ARCHIVE}
if [ -f ${ARCHIVE} ]; then
        unpack_archive ${ARCHIVE}
else
        clone_repo_git ${GITREPO} ${DIRNAME} ${TOPHASH}
fi

setup_source_git ${DIRNAME}

# build
echo $PATH
cd ${DIRNAME}
make CXX=${MPICXX}
