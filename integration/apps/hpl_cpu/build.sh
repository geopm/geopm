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

codebase="netlib"

function usage {
    set +x
    echo "Usage: build.sh [--mkl] [--netlib] [-h |--help]"
    echo "   Default is netlib if not specified."
}

if (( $# > 1 )); then
    set +x
    shift
    usage
    echo "Extra arguments: $@"
    exit 1
fi

while [ "$1" != "" ]; do
    case $1 in
        --netlib )    codebase="netlib"
                      ;;
        --mkl )       codebase="mkl"
                      ;;
        --help | -h ) usage
                      exit
                      ;;
        * )           echo "Argument not known: $1"
                      usage
                      exit 1
    esac
    shift
done

if [[ "${codebase}" == "netlib" ]]; then
    set -x
    # Get helper functions
    source ../build_func.sh

    # Set variables for workload
    DIRNAME=hpl-2.3
    ARCHIVE=${DIRNAME}.tar.gz
    URL=https://www.netlib.org/benchmark/hpl/

    # Run helper functions
    clean_source ${DIRNAME}
    get_archive ${ARCHIVE} ${URL}
    unpack_archive ${ARCHIVE}
    setup_source_git ${DIRNAME}

    # Build application
    cd ${DIRNAME}
    make arch=Linux_Intel64
else
    set +x
    echo "This script will check if:"
    echo "   1. MKL is installed in your system and environment is set (check $MKLROOT environment variable)."
    echo "   2. Intel HPL binary is installed as a part of the MKL installation in the \$MKLROOT/benchmarks/mp_linpack directory."
    echo "   3. Check MPI version."
    echo "It will not perform any download/build. User is expected to install MKL manually."

    echo "Check 1..."
    if [[ "${MKLROOT}" == "" ]]; then
        echo "ERROR: MKLROOT env variable is not found. Make sure that the MKL is installed."
    fi

    echo "Check 2..."
    xhpl_executable=${MKLROOT}/benchmarks/mp_linpack/xhpl_intel64_dynamic
    if ! [[ -f "${xhpl_executable}" ]]; then
        echo "ERROR: HPL executable is not found in the MKL installation at ${xhpl_executable}."
    fi

    xhpl_executable_wrapper=${MKLROOT}/benchmarks/mp_linpack/runme_intel64_prv
    if ! [[ -f "${xhpl_executable_wrapper}" ]]; then
        echo "ERROR: HPL executable wrapper is not found in the MKL installation at ${xhpl_executable_wrapper}."
    fi

    echo "Check 3..."
    mpilib=$(ldd ${xhpl_executable} |grep libmpi.so)
    echo "MPI library: ${mpilib}"
    if echo ${mpilib} |grep mvapich > /dev/null; then
        echo "INFO: Detected MVAPICH MPI. Not recommended for MKL HPL runs."
    fi

    echo "MKL HPL seems to be installed on your system."
fi