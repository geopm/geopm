#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
    --gpu-architecture=sm_70 -rdc=true \
    --expt-extended-lambda -g -O3 -std=c++14 \
    -L$cuda_lib -D_X86INTRIN_H_INCLUDED -DPRKVERSION="2020" nstream-mpi-cuda.cu $mpi_flags -o nstream-mpi-cuda

