#!/bin/bash
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GEOPM BUILD ENVIRONMENT
#
# This script is intended to be sourced within an existing script or shell ONLY.
# It is NOT intended to be ./ executed.

if [ -f ${HOME}/.geopmrc ]; then
    source ${HOME}/.geopmrc
fi

if [ ! -z ${GEOPM_SYSTEM_ENV} ]; then
    source ${GEOPM_SYSTEM_ENV}
fi

# Default GEOPM setup
export GEOPM_SOURCE=${GEOPM_SOURCE:?Please set GEOPM_SOURCE in your environment.}
export GEOPM_INSTALL=${GEOPM_INSTALL:?Please set GEOPM_INSTALL in your environment.}
export GEOPM_APPS_SOURCES=${GEOPM_APPS_SOURCES:?Please set GEOPM_APPS_SOURCES in your environment.}

# Using Intel specific names for MPI compiler wrappers if no other settings
# are provided by the user.
export CC=${CC:-icc}
export CXX=${CXX:-icpc}
export FC=${FC:-ifort}
export F77=${F77:-ifort}
export F90=${F90:-ifort}
export MPICC=${MPICC:-mpiicc}
export MPICXX=${MPICXX:-mpiicpc}
export MPIFORT=${MPIFORT:-mpiifort}
export MPIFC=${MPIFC:-mpiifort}
export MPIF77=${MPIF77:-mpiifort}
export MPIF90=${MPIF90:-mpiifort}

if [ -z ${GEOPM_SKIP_COMPILER_CHECK+x} ]; then
    COMPILER_LIST="CC CXX MPICC MPICXX FC F77 F90 MPIFC MPIFORT MPIF77 MPIF90"
    for compiler in ${COMPILER_LIST}; do
        # Check to ensure Intel (R) is used
        if ! ${!compiler} --version | grep --quiet Intel; then
            echo "Error: Please ensure the Intel (R) toolchain is setup properly. (${compiler} = ${!compiler} is not supported)"
            return 1
        fi
    done
fi

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
export GEOPM_FORTRAN_LDLIBS="${GEOPM_LDLIBS} -lgeopmfort"
