#!/bin/bash

set -x -e
PACKAGE_NAME=geopmpy

./make_sdist.sh

VERSION=$(cat ${PACKAGE_NAME}/VERSION)
ARCHIVE=${PACKAGE_NAME}-${VERSION}.tar.gz

tar -xvf dist/${ARCHIVE}
DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage -us -uc
