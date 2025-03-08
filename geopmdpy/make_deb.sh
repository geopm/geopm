#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -xe

# Build against locally built libgeopmd deb files
BUILD_AGAINST_LOCAL_PACKAGES=1

if [ "$#" -eq 1 ] && [ "$1" == '--no-local' ]
then
    # Build against installed libgeopmddeb files
    BUILD_AGAINST_LOCAL_PACKAGES=0
fi

./make_sdist.sh

PACKAGE_NAME=geopmdpy
ARCHIVE=${PACKAGE_NAME}-$(cat ${PACKAGE_NAME}/VERSION).tar.gz
tar -xvf dist/${ARCHIVE}

if [ "$BUILD_AGAINST_LOCAL_PACKAGES" -eq 1 ] && ! echo "$DPKG_BUILDPACKAGE_OPTIONS" | grep -qw "\-S\|--build=source"
then
    # Compile and link against the most recently packaged libgeopmd
    libgeopmd_dir=$PWD/../libgeopmd
    deps_tmp_root="${PWD}/$(mktemp -d libgeopmd-deps-tmp.XXXXXX)"
    if ! pushd "$deps_tmp_root"
    then
        1>&2 echo "Error: failed to use a temporary directory for libgeopmd dependencies"
        exit 1
    fi

    for deb_path in "${libgeopmd_dir}"/*"$(cat ../../libgeopmd/VERSION)"*.deb
    do
        ar x "$deb_path"
        tar xf data.tar.zst
    done
    export C_INCLUDE_PATH="${PWD}/usr/include"
    export LIBRARY_PATH="$(dirname "$(find $PWD/usr -name libgeopmd.so.2 | head -n1)")"
    popd
fi

DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage ${DPKG_BUILDPACKAGE_OPTIONS:- -us -uc}
cd -

if [ "$BUILD_AGAINST_LOCAL_PACKAGES" -eq 1 ] && ! echo "$DPKG_BUILDPACKAGE_OPTIONS" | grep -qw "\-S\|--build=source"
then
    rm -r "${deps_tmp_root}"
fi
