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


get_conus2_5km_source() {
    ARCHIVE=bench_2.5km.tar.bz2
}

# Setup env variables for workload
export DIR=${GEOPM_SOURCE}/integration/apps/parres/

# Acquire the dependencies:
DIRNAME=Kernels
URL=https://github.com/ParRes/Kernels.git
HASH=89b067b9
clone_repo_git ${URL} ${DIRNAME} ${HASH}

#Setup git and apply patches
setup_source_git ${DIRNAME}

# CD to work area
cd ${DIRNAME}/Cxx11

# Set vars
if [ -z ${I_MPI_ROOT} ]; then
   I_MPI_ROOT = "/opt/intel/compilers_and_libraries/linux/mpi"
fi

if command -v nvcc; then
    nvcc_bin=$(command -v nvcc)
    cuda_lib=$(echo $nvcc_bin | sed -e 's/bin\/mpicc$/lib64/')
else
    nvcc_bin=/usr/local/cuda/bin/nvcc
    cuda_lib=/usr/local/cuda/lib64
fi

gcc_bindir=/usr/bin

# Compile Single GPU non-MPI version
$nvcc_bin \
    --compiler-bindir=$gcc_bindir \
    --gpu-architecture=sm_70 \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib \
    -D_MWAITXINTRIN_H_INCLUDED -DPRKVERSION="2.16" dgemm-cublas.cu -lcublas -o dgemm-cublas

# Compile Single CPU Multi-GPU version
$nvcc_bin \
    --compiler-bindir=$gcc_bindir \
    --gpu-architecture=sm_70 \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib \
    -D_MWAITXINTRIN_H_INCLUDED -DPRKVERSION="2.16" dgemm-multigpu-cublas.cu -lcublas -o dgemm-multigpu-cublas

# Compile Multi GPU MPI version
mpi_flags="-I${I_MPI_ROOT}/intel64/include -L${I_MPI_ROOT}/intel64/lib/release -L${I_MPI_ROOT}/intel64/lib -Xlinker --enable-new-dtags -Xlinker -rpath -Xlinker ${I_MPI_ROOT}/intel64/lib/release -Xlinker -rpath -Xlinker ${I_MPI_ROOT}/intel64/lib -lmpifort -lmpi -lrt -lpthread -Xlinker --enable-new-dtags -ldl"
$nvcc_bin \
    --compiler-bindir=$gcc_bindir \
    --gpu-architecture=sm_70 \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib -D_MWAITXINTRIN_H_INCLUDED -DPRKVERSION="2.16" dgemm-mpi-cublas.cu -lcublas $mpi_flags -o dgemm-mpi-cublas

# Compile MPI version of nstream
$nvcc_bin \
    --compiler-bindir=$gcc_bindir \
    --gpu-architecture=sm_70 \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib \
    -L$cuda_lib -D_MWAITXINTRIN_H_INCLUDED -DPRKVERSION="2.16" nstream-cublas.cu -lcublas $mpi_flags -o nstream-mpi-cublas

# Compile MPI version of sgemm
$nvcc_bin \
    --compiler-bindir=$gcc_bindir \
    --gpu-architecture=sm_70 \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib \
    -L$cuda_lib -D_MWAITXINTRIN_H_INCLUDED -DPRKVERSION="2.16" sgemm-cublas.cu -lcublas $mpi_flags -o sgemm-mpi-cublas

# Compile MPI version of transpose
$nvcc_bin \
    --compiler-bindir=$gcc_bindir \
    --gpu-architecture=sm_70 \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib \
    -L$cuda_lib -D_MWAITXINTRIN_H_INCLUDED -DPRKVERSION="2.16" transpose-cublas.cu -lcublas $mpi_flags -o transpose-mpi-cublas
