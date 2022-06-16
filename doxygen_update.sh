#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -e

SCRIPT_FILE=$0
SCRIPT_DIR=$(realpath $(dirname ${SCRIPT_FILE}))

cd ${SCRIPT_DIR}/service
sed -e "s|@DOX_VERSION@|$(cat VERSION)|g" \
    -e "s|@DOX_OUTPUT@|dox|g" \
    -e "s|@DOX_INPUT@|dox/blurb.md src|g" \
    dox/Doxyfile.in > dox/Doxyfile
doxygen dox/Doxyfile
mkdir -p docs/build/html
rm -rf docs/build/html/geopm-service-dox
cp -rp dox/html docs/build/html/geopm-service-dox

cd ${SCRIPT_DIR}
sed -e "s|@DOX_VERSION@|$(cat VERSION)|g" \
    -e "s|@DOX_OUTPUT@|dox|g" \
    -e "s|@DOX_INPUT@|README.md src|g" \
    dox/Doxyfile.in > dox/Doxyfile
doxygen dox/Doxyfile
rm -rf service/docs/build/html/geopm-runtime-dox
cp -rp dox/html service/docs/build/html/geopm-runtime-dox
