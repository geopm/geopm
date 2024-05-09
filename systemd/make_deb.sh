#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [ $0 != './make_deb.sh' ]; then
    echo "Error: Working directory must be geopm/systemd" 1>&2
    exit -1
fi

set -xe

make dist
VERSION=$(cat VERSION)
tar xvf geopm-service-$VERSION.tar.gz
cd geopm-service-$VERSION
dpkg-buildpackage ${DPKG_BUILDPACKAGE_OPTIONS:- -us -uc}
