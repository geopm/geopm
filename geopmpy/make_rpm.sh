#!/bin/bash

set -xe

./make_sdist.sh

PACKAGE_NAME=geopmpy
VERSION=$(python3 -c "from setuptools_scm import get_version; print(get_version('..'))")
ARCHIVE=${PACKAGE_NAME}-${VERSION}.tar.gz

sed -e "s/@ARCHIVE@/${ARCHIVE}/" -e "s/@VERSION@/${VERSION}/" geopmpy.spec.in > geopmpy.spec

# On systems where rpmbuild is not available (i.e. GitHub CI builder used to create
# the distribution tarball that is uploaded to OBS) the following will be skipped.
if [ -x "$(command -v rpmbuild)" ]; then
    RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
    mkdir -p ${RPM_TOPDIR}/SOURCES
    mkdir -p ${RPM_TOPDIR}/SPECS
    cp dist/${ARCHIVE} ${RPM_TOPDIR}/SOURCES
    cp geopmpy.spec ${RPM_TOPDIR}/SPECS
    rpmbuild -ba ${RPM_TOPDIR}/SPECS/geopmpy.spec
fi
