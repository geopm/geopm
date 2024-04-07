#!/bin/bash

if [ $0 != './make_deb.sh' ]; then
    echo "Error: Working directory must be geopm/docs" 1>&2
    exit -1
fi

set -e

make clean
make dist
VERSION=$(cat VERSION)
tar xvf geopm-docs-$VERSION.tar.gz
cd geopm-docs-$VERSION
dpkg-buildpackage -us -uc
