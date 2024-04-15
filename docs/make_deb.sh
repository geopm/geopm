#!/bin/bash

if [ $0 != './make_deb.sh' ]; then
    echo "Error: Working directory must be geopm/docs" 1>&2
    exit -1
fi

set -xe

make clean_all
make dist
VERSION=$(cat VERSION)
tar xvf geopm-doc-$VERSION.tar.gz
cd geopm-doc-$VERSION
dpkg-buildpackage ${DPKG_BUILDPACKAGE_OPTIONS:-"-us -uc"}
