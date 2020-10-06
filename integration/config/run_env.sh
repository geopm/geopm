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

# Use whichever python version was used to build geopmpy
GEOPMPY_PKGDIR=${GEOPMPY_PKGDIR:-$(ls -dv ${GEOPM_LIB}/python*/site-packages 2>/dev/null)}
if [ 0 -eq $? ]; then
    if [ 1 -ne $(echo "${GEOPMPY_PKGDIR}" | wc -l) ]; then
        GEOPMPY_PKGDIR=$(echo "${GEOPMPY_PKGDIR}" | tail -n1)
        echo 1>&2 "Error: More than 1 python site-packages directory in ${GEOPM_LIB}"
        echo 1>&2 "       Remove all except one, or manually set GEOPMPY_PKGDIR."
        echo 1>&2 "       Assuming GEOPMPY_PKGDIR=${GEOPMPY_PKGDIR}."
        return 1
    fi
else
    echo 1>&2 "Error: Unable to find python site-packages in ${GEOPM_LIB}"
    return 1
fi

# Setup exports
PATH_EXT=${GEOPM_INSTALL}/bin
if [ ! -z "${PATH}" ]; then
    export PATH=${PATH_EXT}:${PATH}
else
    echo "Warning: No PATH set?" 1>&2
    export PATH=${PATH_EXT}
fi

LD_LIBRARY_PATH_EXT=${GEOPM_INSTALL}/lib
if [ ! -z "${LD_LIBRARY_PATH}" ]; then
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_EXT}:${LD_LIBRARY_PATH}
else
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_EXT}
fi

PYTHONPATH_EXT=\
"${GEOPMPY_PKGDIR}:"\
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
    echo "Error: 'geopmread' is not available.  Please build and install GEOPM into GEOPM_BUILD = ${GEOPM_BUILD}."
    return 1
fi

# Check installed version of GEOPM against source version
pushd ${GEOPM_SOURCE} > /dev/null
GEOPM_SOURCE_VERSION=$(cat VERSION)
popd > /dev/null
GEOPMREAD_VERSION=$(geopmread --version | head -n1)

if [ "${GEOPMREAD_VERSION}" != "${GEOPM_SOURCE_VERSION}" ]; then
    echo "Warning: Version mismatch between installed version of GEOPM and the source tree!"
    echo"   Installed version = ${GEOPMREAD_VERSION} | Source version = ${GEOPM_SOURCE_VERSION}"
fi

if ! ldd $(which geopmbench) | grep --quiet libimf; then
    echo "Error: Please build geopm with the Intel Toolchain"
    echo "       to ensure the best performance."
    return 1
fi

