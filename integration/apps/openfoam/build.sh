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

module purge
module load intel impi

NPROCS=${NPROCS:?"Please set NPROCS to the number of parallel build threads to use"}

set -e

# Get helper functions
source ../build_func.sh

# Set variables for workload
DIRNAME=OpenFOAM-v2006
DIRNAME_TPL=ThirdParty-v2006
URL=https://sourceforge.net/projects/openfoam/files/v2006/

clean_source ${DIRNAME}
clean_source ${DIRNAME_TPL}

ARCHIVE=${DIRNAME}.tgz
get_archive ${ARCHIVE} ${URL}
unpack_archive ${ARCHIVE}

ARCHIVE=${DIRNAME_TPL}.tgz
get_archive ${ARCHIVE} ${URL}
unpack_archive ${ARCHIVE}

setup_source_git ${DIRNAME}

# setup environment
export MPI_ROOT=$(which mpiicc | grep -o ".*/mpi/")

# Workaround: bashrc calls scripts that return non-zero
source ${DIRNAME}/etc/bashrc || true

# Build third party
cd ${THIRD_PARTY}
./Allwmake -j $NPROCS |& tee build.log
cd -

# Build OpenFOAM
cd ${DIRNAME}

echo "Beginning compile for OpenFOAM (>1 hour), please wait..."
./Allwmake -j $NPROCS |& tee build.log

echo "...done"
cd -

# Download input generation scripts
git clone https://github.com/OpenFOAM/OpenFOAM-Intel.git
cd OpenFOAM-Intel
git am ../intel_patches/0001-Change-Setup-script-to-use-srun.patch
cd -
