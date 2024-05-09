#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -xe

make dist
RPM_TOPDIR=${RPM_TOPDIR:-${HOME}/rpmbuild}
mkdir -p ${RPM_TOPDIR}/SOURCES
mkdir -p ${RPM_TOPDIR}/SPECS
cp geopm-systemd-$(cat VERSION).tar.gz ${RPM_TOPDIR}/SOURCES
cp geopm-systemd.spec ${RPM_TOPDIR}/SPECS
rpmbuild -ba ${RPM_TOPDIR}/SPECS/geopm-systemd.spec
