#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x -e

./make_sdist.sh

PACKAGE_NAME=geopmpy
ARCHIVE=${PACKAGE_NAME}-$(cat ${PACKAGE_NAME}/VERSION).tar.gz
tar -xvf dist/${ARCHIVE}
DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage ${DPKG_BUILDPACKAGE_OPTIONS:- -us -uc}
cd -
