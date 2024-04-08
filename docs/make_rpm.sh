#!/bin/bash

set -xe

make dist

PACKAGE_NAME=geopm-docs
VERSION=$(cat VERSION)
ARCHIVE=${PACKAGE_NAME}-${VERSION}.tar.gz

sed -e "s/@ARCHIVE@/${ARCHIVE}/" -e "s/@VERSION@/${VERSION}/" geopm-docs.spec.in > geopm-docs.spec

# On systems where rpmbuild is not available (i.e. GitHub CI builder used to create
# the distribution tarball that is uploaded to OBS) the following will be skipped.
if [ -x "$(command -v rpmbuild)" ]; then
    RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
    mkdir -p ${RPM_TOPDIR}/SOURCES
    mkdir -p ${RPM_TOPDIR}/SPECS
    cp ${ARCHIVE} ${RPM_TOPDIR}/SOURCES
    cp geopm-docs.spec ${RPM_TOPDIR}/SPECS
    rpmbuild -ba ${RPM_TOPDIR}/SPECS/geopm-docs.spec
fi
