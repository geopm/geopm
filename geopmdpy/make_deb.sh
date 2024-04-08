#!/bin/bash

set -xe
PACKAGE_NAME=geopmdpy

./make_sdist.sh

VERSION=$(python3 -c "from setuptools_scm import get_version; print(get_version('..'))")
ARCHIVE=${PACKAGE_NAME}-${VERSION}.tar.gz

tar -xvf dist/${ARCHIVE}
DIR=$(echo ${ARCHIVE} | sed 's|\.tar\.gz||')
cd ${DIR}
dpkg-buildpackage -us -uc
