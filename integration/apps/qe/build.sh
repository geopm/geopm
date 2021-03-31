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

XCONFIGURE_DIR="xconfigure-master"
XCONFIGURE_ARCHIVE="master.zip"
ELPA_ARCHIVE="elpa-2020.05.001.tar.gz"
ELPA_DIR="elpa-2020.05.001"
QE_ARCHIVE="q-e-qe-6.6.tar.bz2"
ES_BENCH_DIR="es-benchmarks"
ES_BENCH_GIT="https://github.com/electronic-structure/benchmarks.git"
ES_BENCH_GIT_HASH="b0069fc"
ES_BENCH_ARCHIVE="${ES_BENCH_DIR}_${ES_BENCH_GIT_HASH}.tgz"
QEF_BENCH_DIR="qef-benchmarks"
QEF_BENCH_GIT="https://github.com/QEF/benchmarks.git"
QEF_BENCH_GIT_HASH="0ddeb56"
QEF_BENCH_ARCHIVE="${QEF_BENCH_DIR}_${QEF_BENCH_GIT_HASH}.tgz"
QE_DIR="q-e-qe-6.6"

clean_source ${QE_DIR}
clean_source ${ELPA_DIR}
clean_source ${ES_BENCH_DIR}
clean_source ${QEF_BENCH_DIR}
clean_source ${XCONFIGURE_DIR}

# Acquire the build modifiers for ELPA and QE
# Usage described at https://xconfigure.readthedocs.io/en/latest/qe/
get_archive ${XCONFIGURE_ARCHIVE} "https://github.com/hfp/xconfigure/archive"
unpack_archive ${XCONFIGURE_ARCHIVE}

# Acquire the ELPA dependency
get_archive ${ELPA_ARCHIVE} "https://elpa.mpcdf.mpg.de/html/Releases/2020.05.001"
unpack_archive ${ELPA_ARCHIVE}
setup_source_git ${ELPA_DIR}
cp -r ${XCONFIGURE_DIR}/config/elpa/* ${ELPA_DIR}

# Acquire the QE source
get_archive ${QE_ARCHIVE} "https://gitlab.com/QEF/q-e/-/archive/qe-6.6"
unpack_archive ${QE_ARCHIVE}
setup_source_git ${QE_DIR}
cp -r ${XCONFIGURE_DIR}/config/qe/* ${QE_DIR}

# Get the benchmark inputs
get_archive ${ES_BENCH_ARCHIVE}
if [ -f ${ES_BENCH_ARCHIVE} ]; then
    unpack_archive ${ES_BENCH_ARCHIVE}
else
    clone_repo_git ${ES_BENCH_GIT} ${ES_BENCH_DIR} ${ES_BENCH_GIT_HASH}
fi
setup_source_git ${ES_BENCH_DIR}

get_archive ${QEF_BENCH_ARCHIVE}
if [ -f ${QEF_BENCH_ARCHIVE} ]; then
    unpack_archive ${QEF_BENCH_ARCHIVE}
else
    clone_repo_git ${QEF_BENCH_GIT} ${QEF_BENCH_DIR} ${QEF_BENCH_GIT_HASH}
fi
setup_source_git ${QEF_BENCH_DIR}

# Build ELPA dependency
pushd ${ELPA_DIR}
make clean || ./configure-elpa-skx-omp.sh
make -j
# Install to ../elpa/intel-skx-omp
make install
popd

# Build QE
pushd ${QE_DIR}
make distclean || ./configure-qe-skx-omp.sh
make pw -j
popd
