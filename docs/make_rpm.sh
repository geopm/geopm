#!/bin/bash

set -xe

make clean_all
make dist
RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
mkdir -p ${RPM_TOPDIR}/SOURCES
mkdir -p ${RPM_TOPDIR}/SPECS
cp ${ARCHIVE} ${RPM_TOPDIR}/SOURCES
cp geopm-docs.spec ${RPM_TOPDIR}/SPECS
rpmbuild -ba ${RPM_TOPDIR}/SPECS/geopm-docs.spec
