#!/bin/bash
#
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

# GEOPM BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

if [ -f ${HOME}/.geopmrc ]; then
    source ~/.geopmrc
fi

if [ ! -z ${GEOPM_SYSTEM_ENV} ]; then
    source ${GEOPM_SYSTEM_ENV}
fi

# Default GEOPM setup
export GEOPM_SOURCE=${GEOPM_SOURCE:?Please set GEOPM_SOURCE in your environment.}
export GEOPM_INSTALL=${GEOPM_INSTALL:?Please set GEOPM_INSTALL in your environment.}
export GEOPM_APPS_SOURCES=${GEOPM_APPS_SOURCES:?Please set GEOPM_APPS_SOURCES in your environment.}

# Default Intel Toolchain compiler overrides
export CC=${CC:-icc}
export CXX=${CXX:-icpc}
export MPICC=${MPICC:-mpicc}
export MPICXX=${MPICXX:-mpicxx}
export FC=${FC:-ifort}
export F77=${F77:-ifort}
export MPIFC=${MPIFC:-mpifort}
export MPIF77=${MPIF77:-mpifort}

COMPILER_LIST="CC CXX MPICC MPICXX FC F77 MPIFC MPIF77"
for compiler in ${COMPILER_LIST}; do
    # Check to ensure Intel is used
    if ! ${!compiler} --version | grep --quiet Intel; then
        echo "Error: Please ensure the Intel Toolchain is setup properly. (${compiler} = ${!compiler} is not supported)"
        return 1
    fi
done

# lmod-like exports
if [ ! -z "${MANPATH}" ]; then
    export MANPATH="${GEOPM_INSTALL}/share/man":${MANPATH}
else
    export MANPATH="${GEOPM_INSTALL}/share/man"
fi

export GEOPM_INC=${GEOPM_INSTALL}/include
export GEOPM_LIB=${GEOPM_INSTALL}/lib
export GEOPM_CFLAGS="-I${GEOPM_INC}"
export GEOPM_FFLAGS="-I${GEOPM_LIB}/${FC}/modules/geopm-x86_64"
export GEOPM_LDFLAGS="-L${GEOPM_LIB}"
export GEOPM_LDLIBS="-lgeopm"
export GEOPM_FORTRAN_LDLIBS="${GEOPM_LDLIBS} -lgeopmfortran"

