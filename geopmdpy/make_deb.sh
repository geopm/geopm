#!/bin/bash

set -xe

./make_sdist.sh

PACKAGE_NAME=geopmdpy
VERSION=$(cat geopmdpy/VERSION)
ARCHIVE=${PACKAGE_NAME}-${VERSION}.tar.gz

tar -xvf dist/${ARCHIVE}
DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage -us -uc
