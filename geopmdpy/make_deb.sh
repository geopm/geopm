#!/bin/bash

set -xe

./make_sdist.sh

PACKAGE_NAME=geopmdpy
ARCHIVE=${PACKAGE_NAME}-$(cat ${PACKAGE_NAME}/VERSION).tar.gz
tar -xvf dist/${ARCHIVE}
DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage ${DPKG_BUILDPACKAGE_OPTIONS:-"-us -uc"}
cd -
