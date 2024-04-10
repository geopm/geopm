#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Simple script that will build and test both the runtime and the service.

set -e

usage(){
    echo "Usage:"
    echo "    <ENVIRONMENT_OVERRIDES> ${0}"
    echo
    echo "Requirements:"
    echo "    GEOPM_SOURCE and GEOPM_INSTALL are set in the calling environment."
    echo "    Ideally this is done with ~/.geopmrc."
    echo "    See integration/README.md for more information."
    echo
    echo "ENVIRONMENT_OVERRIDES allows for the following variables to be specified:"
    echo "    GEOPM_GLOBAL_CONFIG_OPTIONS : Configure options applicable to all build types."
    echo "    GEOPM_RUNTIME_CONFIG_OPTIONS : Configure options only applicable to the runtime build."
    echo "    GEOPM_SERVICE_CONFIG_OPTIONS : Configure options only applicable to the service build."
    echo "        Ensure quotes are used when specifying multiple options.  See examples below."
    echo "    GEOPM_OBJDIR: Build time objects go here (e.g. Makefile, processed .in files)."
    echo "    GEOPM_NUM_THREADS : The number of threads to use when running make (default: 9)."
    echo "    GEOPM_RUN_TESTS : Set to enable running of unit tests."
    echo "    GEOPM_SKIP_SERVICE_INSTALL : Set to skip \"make install\" of the service."
    echo "    GEOPM_SKIP_RUNTIME_INSTALL : Set to skip \"make install\" of the runtime directory (HPC runtime)."
    echo
    echo "Examples (proceeded by \"git clean -ffdx\" to ensure the build tree is clean):"
    echo "    GEOPM_GLOBAL_CONFIG_OPTIONS=\"--enable-debug --enable-coverage\" GEOPM_RUN_TESTS=yes ./build.sh"
    echo "    GEOPM_RUNTIME_CONFIG_OPTIONS=\"--enable-beta\" ./build.sh"
    echo "    GEOPM_SERVICE_CONFIG_OPTIONS=\"--enable-levelzero\" ./build.sh"
    echo "    GEOPM_SKIP_SERVICE_INSTALL=yes GEOPM_SERVICE_CONFIG_OPTIONS=\"--disable-systemd --disable-docs\" integration/config/build.sh"
}

if [[ ${1} == '--help' ]]; then
    usage
    exit 1
fi

GEOPM_SOURCE=${GEOPM_SOURCE:-${PWD}}
cd ${GEOPM_SOURCE}
BUILD_ENV="integration/config/build_env.sh"
if [ ! -f "${BUILD_ENV}" ]; then
    usage
    exit 1
fi
source ${BUILD_ENV}

# Check that GEOPM_INSTALL is defined unless we are skipping install
# This check is redundant with the check that is in build_env.sh
# that enforces GEOPM_INSTALL to be set.
# We must check 2 conditions:
#   1. If GEOPM_INSTALL is not set, both of the skip install options must be used.
#   2. If *one* of the skip install options is set, GEOPM_INSTALL must be set.
if [ -z "${GEOPM_INSTALL}" ] && \
   [ -z "${GEOPM_SKIP_RUNTIME_INSTALL+x}" ] && \
   [ -z "${GEOPM_SKIP_SERVICE_INSTALL+x}" ]; then
    echo "Please define GEOPM_INSTALL or GEOPM_SKIP_RUNTIME_INSTALL *and* GEOPM_SKIP_SERVICE_INSTALL prior to using this script" 1>&2
    exit 1
fi

if [ ! -z "${GEOPM_SKIP_SERVICE_INSTALL+x}" ] || \
   [ ! -z "${GEOPM_SKIP_RUNTIME_INSTALL+x}" ] && \
   [ -z "${GEOPM_INSTALL}" ]; then
    echo "Please define GEOPM_INSTALL prior to using this script unless you meant to specify GEOPM_SKIP_SERVICE_INSTALL *and* GEOPM_SKIP_RUNTIME_INSTALL" 1>&2
    exit 1
fi

# Clear out old builds unless we are skipping install
if [ -z "${GEOPM_SKIP_SERVICE_INSTALL+x}" ] || \
   [ -z "${GEOPM_SKIP_RUNTIME_INSTALL+x}" ] && \
   [ -d "${GEOPM_INSTALL}" ]; then
    echo "Removing old build located at ${GEOPM_INSTALL}"
    rm -rf ${GEOPM_INSTALL}
    set +e
    # The following line will fail if there is a system install,
    # but no user install.  This is OK.
    python3 -m pip uninstall -y geopmdpy 2> /dev/null
    set -e
    python3 -m pip uninstall -y geopmpy
fi

# Append prefix to config options unless skipping install
if [ -z "${GEOPM_SKIP_SERVICE_INSTALL+x}" ]; then
    GEOPM_SERVICE_CONFIG_OPTIONS="${GEOPM_SERVICE_CONFIG_OPTIONS} --prefix=${GEOPM_INSTALL}"
fi
if [ -z "${GEOPM_SKIP_RUNTIME_INSTALL+x}" ]; then
    GEOPM_RUNTIME_CONFIG_OPTIONS="${GEOPM_RUNTIME_CONFIG_OPTIONS} --prefix=${GEOPM_INSTALL}"
fi

# Combine global options with build specific options
GEOPM_RUNTIME_CONFIG_OPTIONS="${GEOPM_RUNTIME_CONFIG_OPTIONS} ${GEOPM_GLOBAL_CONFIG_OPTIONS}"
GEOPM_SERVICE_CONFIG_OPTIONS="${GEOPM_SERVICE_CONFIG_OPTIONS} ${GEOPM_GLOBAL_CONFIG_OPTIONS}"

GEOPM_NUM_THREAD=${GEOPM_NUM_THREAD:-9}

build(){
    local BUILDROOT=${PWD}
    local CONFIG_OPTS=${1}
    local GEOPM_SKIP_INSTALL=${2}
    if [ ! -f "configure" ]; then
        ./autogen.sh
    fi

    if [ ! -z ${GEOPM_OBJDIR} ]; then
        mkdir -p ${GEOPM_OBJDIR} # Objects created at configure time will go here
        cd ${GEOPM_OBJDIR}
    fi

    if [ ! -f "Makefile" ]; then
        if [ ! -z ${GEOPM_OBJDIR} ]; then
            ${BUILDROOT}/configure ${CONFIG_OPTS}
        else
            ./configure ${CONFIG_OPTS}
        fi
    fi
    make -j${GEOPM_NUM_THREAD}

    # By default, the tests are skipped
    if [ ! -z ${GEOPM_RUN_TESTS} ]; then
        make -j${GEOPM_NUM_THREAD} checkprogs
        make check
    fi

    # By default, make install is called
    if [ -z ${GEOPM_SKIP_INSTALL} ]; then
        make install
    fi
}


# Run the service build
cd libgeopmd
unset CFLAGS CXXFLAGS
CC=gcc CXX=g++ build "${GEOPM_SERVICE_CONFIG_OPTIONS}" ${GEOPM_SKIP_SERVICE_INSTALL}

# Run the runtime build
cd ${GEOPM_SOURCE}/libgeopm

if [ ! -z ${GEOPM_OBJDIR} ]; then
    build "--with-geopmd-lib=${GEOPM_SOURCE}/service/${GEOPM_OBJDIR}/.libs \
           --with-geopmd-include=${GEOPM_SOURCE}/service/src \
           ${GEOPM_RUNTIME_CONFIG_OPTIONS}" ${GEOPM_SKIP_RUNTIME_INSTALL}
else
    build "${GEOPM_RUNTIME_CONFIG_OPTIONS}" ${GEOPM_SKIP_RUNTIME_INSTALL}
fi

# Build/Install geopmdpy
if [ -z ${GEOPM_SKIP_SERVICE_INSTALL} ]; then
    cd ${GEOPM_SOURCE}/geopmdpy
    python3 -m build
    python3 -m pip install --user dist/geopmdpy-*.whl
fi

# Build/Install geopmpy
if [ -z ${GEOPM_SKIP_RUNTIME_INSTALL} ]; then
    cd ${GEOPM_SOURCE}/geopmpy
    python3 -m build
    python3 -m pip install --user dist/geopmpy-*.whl
fi

# Build the integration tests, apps, and other examples
cd ${GEOPM_SOURCE}/integration

if [ ! -z ${GEOPM_OBJDIR} ]; then
    build "--with-geopmd-lib=${GEOPM_SOURCE}/service/${GEOPM_OBJDIR}/.libs \
           --with-geopmd-include=${GEOPM_SOURCE}/service/src \
           ${GEOPM_RUNTIME_CONFIG_OPTIONS}" true
else
    build "${GEOPM_RUNTIME_CONFIG_OPTIONS}" true
fi
