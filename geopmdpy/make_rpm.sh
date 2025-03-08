#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -xe

die() {
    1>&2 printf "%s\n" "$@"
    exit 1
}

./make_sdist.sh

PACKAGE_NAME=geopmd
ARCHIVE=geopmdpy-$(cat geopmdpy/VERSION).tar.gz
RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
mkdir -p ${RPM_TOPDIR}/SOURCES
mkdir -p ${RPM_TOPDIR}/SPECS
cp dist/${ARCHIVE} ${RPM_TOPDIR}/SOURCES
cp ${PACKAGE_NAME}.spec ${RPM_TOPDIR}/SPECS

# Use a compiler that produces libraries compatible with the system python. Needed for CFFI.
export CC=gcc

# Compile and link against the most recently packaged libgeopmd
deps_tmp_root="${PWD}/$(mktemp -d libgeopmd-deps-tmp.XXXXXX)"
if ! pushd "$deps_tmp_root"
then
    die "Error: failed to use a temporary directory for libgeopmd dependencies"
fi

libgeopmd_version="$(cat ../../libgeopmd/VERSION)"
if [ -z "${libgeopmd_version}" ]
then
    die "Error: ../../libgeopmd/VERSION is not set"
fi

for rpm_path in ${RPM_TOPDIR}/RPMS/$(uname -m)/libgeopmd*"${libgeopmd_version}"*.rpm
do
    if ! rpm2cpio "$rpm_path" | cpio -idmv
    then
        die "Error: Unable to unpack libgeopmd version ${libgeopmd_version} RPMs" \
            "       Ensure that RPMs have been built in libgeopmd (e.g., run 'make rpm' in libgeopmd)"
    fi
done
popd
export C_INCLUDE_PATH="$deps_tmp_root/usr/include"
export LIBRARY_PATH="$deps_tmp_root/usr/lib:$deps_tmp_root/usr/lib64"

rpmbuild -ba ${RPM_TOPDIR}/SPECS/${PACKAGE_NAME}.spec
rm -r "${deps_tmp_root}"
