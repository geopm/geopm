#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

set -e
set -x

source ../build_func.sh

TARGETS="mcfly quartz xeon"
if [ $# -ne 1 ]; then
    echo "Usage: $0 <target system>.  Available targets: ${TARGETS}"
    exit 1
fi

TARGET_ARCH=${1}

# Supported targets
if [ "${TARGET_ARCH}" = "mcfly" ]; then
    BUILD_PATCHES=""
    TARGET=IMPI_IOMP_SKX
    BIN=xhpcg_skx
elif [ "${TARGET_ARCH}" = "quartz" ]; then
    BUILD_PATCHES="../0001-Change-MPI-compiler-to-mpicxx.patch"
    TARGET=IMPI_IOMP_AVX2
    BIN=xhpcg_avx2
elif [ "${TARGET_ARCH}" = "xeon" ]; then
    BUILD_PATCHES=""
    TARGET=IMPI_IOMP_AVX2
    BIN=xhpcg_avx2
else
    echo "Unknown target.  Available targets: ${TARGETS}"
    exit 1
fi

DIRNAME=hpcg
clean_source ${DIRNAME}
# Acquire the source:
cp -r ${MKLROOT}/benchmarks/hpcg/ ${DIRNAME}
# Get rid of prebuilt binaries
rm ${DIRNAME}/bin/xhpcg*
setup_source_git "${DIRNAME}" "${BUILD_PATCHES}"
# Build
cd ${DIRNAME}
./configure ${TARGET}
make
# Use uniform binary name
cd bin
ln -fs ${BIN} xhpcg.x
