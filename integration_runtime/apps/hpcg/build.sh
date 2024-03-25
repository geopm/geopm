#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
    TARGET=IMPI_IOMP_SKX
    BIN=xhpcg_skx
elif [ "${TARGET_ARCH}" = "quartz" ]; then
    TARGET=IMPI_IOMP_AVX2
    BIN=xhpcg_avx2
elif [ "${TARGET_ARCH}" = "xeon" ]; then
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
setup_source_git ${DIRNAME}
# Build
cd ${DIRNAME}
./configure ${TARGET}
make
# Use uniform binary name
cd bin
ln -fs ${BIN} xhpcg.x
