#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -xe

./make_sdist.sh

PACKAGE_NAME=geopmdpy
ARCHIVE=${PACKAGE_NAME}-$(cat ${PACKAGE_NAME}/VERSION).tar.gz
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
    1>&2 echo "Error: failed to use a temporary directory for libgeopmd dependencies"
    exit 1
fi

for rpm_path in $HOME/rpmbuild/RPMS/$(uname -m)/{geopm-service,libgeopmd2}*"$(cat ../../libgeopmd/VERSION)"*.rpm
do
    rpm2cpio "$rpm_path" | cpio -idmv
done
popd
export C_INCLUDE_PATH="$deps_tmp_root/usr/include"
export LIBRARY_PATH="$deps_tmp_root/usr/lib:$deps_tmp_root/usr/lib64"

rpmbuild -ba ${RPM_TOPDIR}/SPECS/${PACKAGE_NAME}.spec
rm -r "${deps_tmp_root}"
