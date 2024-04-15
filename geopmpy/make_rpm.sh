#!/bin/bash

set -xe

./make_sdist.sh

PACKAGE_NAME=geopmpy
ARCHIVE=${PACKAGE_NAME}-$(cat VERSION).tar.gz
RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
mkdir -p ${RPM_TOPDIR}/SOURCES
mkdir -p ${RPM_TOPDIR}/SPECS
cp dist/${ARCHIVE} ${RPM_TOPDIR}/SOURCES
cp ${PACKAGE_NAME}.spec ${RPM_TOPDIR}/SPECS
rpmbuild -ba ${RPM_TOPDIR}/SPECS/${PACKAGE_NAME}.spec
