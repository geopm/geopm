#!/bin/bash

set -x -e

./make_sdist.sh

PACKAGE_NAME=geopmpy
ARCHIVE=${PACKAGE_NAME}-$(cat VERSION).tar.gz
tar -xvf dist/${ARCHIVE}
DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage -us -uc
