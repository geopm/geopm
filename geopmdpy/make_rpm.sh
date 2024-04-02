#!/bin/bash

set -x -e

# Build source distribution
python3 -m build --sdist | tee make_deb-sdist.log
ARCHIVE=$(cat make_deb-sdist.log | tail -n 1 | sed 's|^Successfully built ||')

RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
mkdir -p ${RPM_TOPDIR}/SOURCES
mkdir -p ${RPM_TOPDIR}/SPECS
cp ${ARCHIVE} ${RPM_TOPDIR}/SOURCES
cp geopmdpy.spec ${RPM_TOPDIR}/SPECS
rpmbuild --define "archive ${ARCHIVE}" -ba ${RPM_TOPDIR}/SPECS/geopmdpy.spec
