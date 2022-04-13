#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Simple script that will build and test both the base and the service.

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
    echo "    GEOPM_BASE_CONFIG_OPTIONS : Configure options only applicable to the base build."
    echo "    GEOPM_SERVICE_CONFIG_OPTIONS : Configure options only applicable to the service build."
    echo "        Ensure quotes are used when specifying multiple options.  See examples below."
    echo "    GEOPM_OBJDIR: Build time objects go here (e.g. Makefile, processed .in files)."
    echo "    GEOPM_NUM_THREADS : The number of threads to use when running make (default: 9)."
    echo "    GEOPM_RUN_TESTS : Set to enable running of unit tests."
    echo "    GEOPM_SKIP_INSTALL : Set to skip \"make install\"."
    echo
    echo "Examples (proceeded by \"git clean -ffdx\" to ensure the build tree is clean):"
    echo "    GEOPM_GLOBAL_CONFIG_OPTIONS=\"--enable-debug --enable-coverage\" GEOPM_RUN_TESTS=yes ./build.sh"
    echo "    GEOPM_BASE_CONFIG_OPTIONS=\"--enable-beta\" ./build.sh"
    echo "    GEOPM_SERVICE_CONFIG_OPTIONS=\"--enable-levelzero --enable-systemd\" ./build.sh"
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
if [ -z "${GEOPM_INSTALL}" ] && [ -z "${GEOPM_SKIP_INSTALL+x}" ]; then
    echo "Please define GEOPM_INSTALL or GEOPM_SKIP_INSTALL prior to using this script" 1>&2
    exit 1
fi

# Clear out old builds unless we are skipping install
if [ -d "${GEOPM_INSTALL}" ] && [ -z "${GEOPM_SKIP_INSTALL+x}" ]; then
    echo "Removing old build located at ${GEOPM_INSTALL}"
    rm -rf ${GEOPM_INSTALL}
fi

# Append prefix to global config options unless skipping install
if [ -z "${GEOPM_SKIP_INSTALL+x}" ]; then
    GEOPM_GLOBAL_CONFIG_OPTIONS="${GEOPM_GLOBAL_CONFIG_OPTIONS} --prefix=${GEOPM_INSTALL}"
fi

# Combine global options with build specific options
GEOPM_BASE_CONFIG_OPTIONS="${GEOPM_BASE_CONFIG_OPTIONS} ${GEOPM_GLOBAL_CONFIG_OPTIONS}"
GEOPM_SERVICE_CONFIG_OPTIONS="${GEOPM_SERVICE_CONFIG_OPTIONS} ${GEOPM_GLOBAL_CONFIG_OPTIONS}"

GEOPM_NUM_THREAD=${GEOPM_NUM_THREAD:-9}

build(){
    BUILDROOT=${PWD}
    if [ ! -f "configure" ]; then
        ./autogen.sh
    fi

    if [ ! -z ${GEOPM_OBJDIR+x} ]; then
        mkdir -p ${GEOPM_OBJDIR} # Objects created at configure time will go here
        cd ${GEOPM_OBJDIR}
    fi

    if [ ! -f "Makefile" ]; then
        if [ ! -z ${GEOPM_OBJDIR+x} ]; then
            ${BUILDROOT}/configure ${1}
        else
            ./configure ${1}
        fi
    fi
    make -j${GEOPM_NUM_THREAD}

    # By default, the tests are skipped
    if [ ! -z ${GEOPM_RUN_TESTS+x} ]; then
        make -j${GEOPM_NUM_THREAD} checkprogs
        make check
    fi

    # By default, make install is called
    if [ -z ${GEOPM_SKIP_INSTALL+x} ]; then
        make install
    fi
}


# Run the service build
cd service
CFLAGS= CXXFLAGS= CC=gcc CXX=g++ build "${GEOPM_SERVICE_CONFIG_OPTIONS}"

# Run the base build
cd ${GEOPM_SOURCE}
if [ -z ${GEOPM_SKIP_INSTALL+x} ]; then
    build "--with-geopmd=${GEOPM_INSTALL} ${GEOPM_BASE_CONFIG_OPTIONS}"
else
    if [ ! -z ${GEOPM_OBJDIR+x} ]; then
        build "--with-geopmd-lib=${GEOPM_SOURCE}/service/${GEOPM_OBJDIR}/.libs \
               --with-geopmd-include=${GEOPM_SOURCE}/service/src \
               ${GEOPM_BASE_CONFIG_OPTIONS}"
    else
        build "${GEOPM_BASE_CONFIG_OPTIONS}"
    fi
fi
