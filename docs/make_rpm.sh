#!/bin/bash

set -xe

make clean_all
make dist
RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
mkdir -p ${RPM_TOPDIR}/SOURCES
mkdir -p ${RPM_TOPDIR}/SPECS
cp geopm-doc-$(cat VERSION).tar.gz ${RPM_TOPDIR}/SOURCES
cp geopm-doc.spec ${RPM_TOPDIR}/SPECS
rpmbuild -ba ${RPM_TOPDIR}/SPECS/geopm-doc.spec
