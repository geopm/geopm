#!/bin/bash
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -e

SCRIPT_FILE=$0
SCRIPT_DIR=$(realpath $(dirname ${SCRIPT_FILE}))

cd ${SCRIPT_DIR}
mkdir -p dox/libgeopmd
sed -e "s|@DOX_VERSION@|$(cat VERSION)|g" \
    -e "s|@DOX_OUTPUT@|dox/libgeopmd|g" \
    -e "s|@DOX_INPUT@|../README.md ../libgeopmd/src ../libgeopmd/include|g" \
    dox/Doxyfile.in > dox/libgeopmd/Doxyfile
doxygen dox/libgeopmd/Doxyfile
rm -rf build/html/geopm-service-dox
cp -rp dox/libgeopmd/html ..//docs/build/html/geopm-service-dox

mkdir -p dox/libgeopm
sed -e "s|@DOX_VERSION@|$(cat VERSION)|g" \
    -e "s|@DOX_OUTPUT@|dox/libgeopm|g" \
    -e "s|@DOX_INPUT@|../README.md ../libgeopm/src ../libgeopm/include|g" \
    dox/Doxyfile.in > dox/libgeopm/Doxyfile
doxygen dox/libgeopm/Doxyfile
rm -rf build/html/geopm-runtime-dox
cp -rp dox/libgeopm/html ..//docs/build/html/geopm-runtime-dox
