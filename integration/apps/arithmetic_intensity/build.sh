#!/bin/bash
#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

set -x
set -e

# Get helper functions
source ../build_func.sh

if [[ $# -eq 0 ]]; then
    echo 'Building from release'
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
if [[ -z "$TOPHASH" ]]; then
    # If a top hash is not specified, get the release version.
    ARCHIVED_DIRNAME=arithmetic-intensity-1.0
    ARCHIVE=v1.0.tar.gz
    URL="${GITREPO}/archive/refs/tags/"
    get_archive ${ARCHIVE} ${URL}
    unpack_archive ${ARCHIVE}
    mv "${ARCHIVED_DIRNAME}" "${DIRNAME}"
else
    # If a top hash is specified, get the latest commit.
    ARCHIVE=${DIRNAME}_${TOPHASH}.tgz
    get_archive ${ARCHIVE}
    if [ -f ${ARCHIVE} ]; then
        unpack_archive ${ARCHIVE}
    else
        clone_repo_git ${GITREPO} ${DIRNAME} ${TOPHASH}
    fi
fi

setup_source_git ${DIRNAME}

# build
echo $PATH
cd ${DIRNAME}
make CXX=${MPICXX}
