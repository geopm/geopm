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

TARGETS="mcfly quartz xeon"
if [ $# -ne 1 ]; then
    echo "Usage: $0 <target system>.  Available targets: ${TARGETS}"
    exit 1
fi

target_arch=$1

HPCG_DIR=hpcg_mkl
if [ -d "$HPCG_DIR" ]; then
    echo "WARNING: Previous HPCG checkout detected at ./$HPCG_DIR"
    read -p "OK to delete and rebuild? (y/n) " -n 1 -r
    echo
    if [[ ${REPLY} =~ ^[Yy]$ ]]; then
        rm -rf $HPCG_DIR
    else
        echo "Not OK.  Stopping."
        exit 1
    fi
fi

# Supported targets
if [ "$target_arch" = "mcfly" ]; then
    BUILD_PATCHES=""
    TARGET=IMPI_IOMP_SKX
    BIN=xhpcg_skx
elif [ "$target_arch" = "quartz" ]; then
    BUILD_PATCHES="../0001-Change-MPI-compiler-to-mpicxx.patch"
    TARGET=IMPI_IOMP_AVX2
    BIN=xhpcg_avx2
elif [ "$target_arch" = "xeon" ]; then
    BUILD_PATCHES=""
    TARGET=IMPI_IOMP_AVX2
    BIN=xhpcg_avx2
else
    echo "Unknown target.  Available targets: ${TARGETS}"
    exit 1
fi

# Acquire the source:
cp -r ${MKLROOT}/benchmarks/hpcg/ $HPCG_DIR

cd $HPCG_DIR

# Set up git
rm bin/xhpcg*
git init
git add -A
git commit --no-edit -s -m "Initial commit"

# Apply patches
git am ${BUILD_PATCHES}

# Build
./configure ${TARGET}
make

# Use uniform binary name
cd bin
ln -fs ${BIN} xhpcg.x
