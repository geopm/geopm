#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GEOPM RUNTIME ENVIRONMENT
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
GEOPM_SOURCE=${GEOPM_SOURCE:?Please set GEOPM_SOURCE in your environment.} # For integration scripts
GEOPM_INSTALL=${GEOPM_INSTALL:?Please set GEOPM_INSTALL in your environment.}
GEOPM_WORKDIR=${GEOPM_WORKDIR:?Please set GEOPM_WORKDIR in your environment.}
GEOPM_LIB=${GEOPM_INSTALL}/lib

# Setup exports
PATH_EXT=${GEOPM_INSTALL}/bin
if [ ! -z "${PATH}" ]; then
    export PATH=${PATH_EXT}:${PATH}
else
    echo "Warning: No PATH set?" 1>&2
    export PATH=${PATH_EXT}
fi

# "${GEOPM_INSTALL}/$(rpm --eval '%{_lib}')" would mimic our rpm packaging, but
# the correct usage for this script depends on what the user ran in ./configure
if [ -e "${GEOPM_INSTALL}/lib64/libgeopm.so" ]; then
    LD_LIBRARY_PATH_EXT=${GEOPM_INSTALL}/lib64
else
    LD_LIBRARY_PATH_EXT=${GEOPM_INSTALL}/lib
fi

if [ ! -z "${LD_LIBRARY_PATH}" ]; then
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_EXT}:${LD_LIBRARY_PATH}
else
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_EXT}
fi

PYTHONPATH_EXT=\
"${GEOPM_SOURCE}/integration"
if [ ! -z "${PYTHONPATH}" ]; then
    export PYTHONPATH=${PYTHONPATH_EXT}:${PYTHONPATH}
else
    export PYTHONPATH=${PYTHONPATH_EXT}
fi

MANPATH_EXT=${GEOPM_INSTALL}/share/man
if [ ! -z "${MANPATH}" ]; then
    export MANPATH=${MANPATH_EXT}:${MANPATH}
else
    export MANPATH=${MANPATH_EXT}
fi

# Sanity checks
if [ ! -d ${GEOPM_WORKDIR} ]; then
    echo "Error: Job output is expected to go in ${GEOPM_WORKDIR} which doesn't exist."
    return 1
fi

if [ ! -x "$(command -v geopmread)" ]; then
    echo "Error: 'geopmread' is not available.  Please build and install GEOPM into GEOPM_INSTALL = ${GEOPM_INSTALL}."
    return 1
fi

# Check installed version of GEOPM against source version
cd ${GEOPM_SOURCE}
GEOPM_SOURCE_VERSION=$(cat VERSION)
cd -
GEOPMREAD_VERSION=$(geopmread --version | head -n1)

if [ "${GEOPMREAD_VERSION}" != "${GEOPM_SOURCE_VERSION}" ]; then
    echo "Warning: Version mismatch between installed version of GEOPM and the source tree!"
    echo "   Installed version = ${GEOPMREAD_VERSION} | Source version = ${GEOPM_SOURCE_VERSION}"
fi

if ! ldd $(which geopmbench) | grep --quiet libimf; then
    echo "Error: Please build geopm with the Intel Toolchain"
    echo "       to ensure the best performance."
    return 1
fi

