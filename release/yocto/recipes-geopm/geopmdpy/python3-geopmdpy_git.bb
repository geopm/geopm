# Copyright (c) 2015 - 2025 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause

SUMMARY = "GEOPM - Python service (geopmd) and client tools"
HOMEPAGE = "https://geopm.github.io"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE-BSD-3-Clause;md5=d36f2aa7fd5b17cb662c664d6bc3f870"

PV = "3.2.0"
SRCREV = "68f4ccce210f63b24d941b6373de29b839cf3711"
SRC_URI = "git://github.com/geopm/geopm;branch=refs/tags/v3.2.0;protocol=https"
S = "${WORKDIR}/git/geopmdpy"

inherit python_pep517 pkgconfig systemd

DEPENDS += "libgeopmd python3-cffi-native python3-setuptools-native python3-setuptools-scm-native"
RDEPENDS:${PN} += " \
    python3-core \
    python3-cffi \
    python3-dasbus \
    python3-jsonschema \
    python3-prometheus-client \
    python3-psutil \
    libgeopmd \
    ${@bb.utils.contains('DISTRO_FEATURES','systemd','systemd','',d)} \
"

# Install the systemd unit and DBus policy
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "geopm.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    # Install systemd unit
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/geopm.service ${D}${systemd_system_unitdir}/geopm.service

    # Install DBus policy and interface XML so distros can generate policies
    install -d ${D}${datadir}/dbus-1/system.d
    if [ -f ${S}/io.github.geopm.conf ]; then
        install -m 0644 ${S}/io.github.geopm.conf ${D}${datadir}/dbus-1/system.d/io.github.geopm.conf
    fi

    install -d ${D}${datadir}/dbus-1/interfaces
    if [ -f ${S}/io.github.geopm.xml ]; then
        install -m 0644 ${S}/io.github.geopm.xml ${D}${datadir}/dbus-1/interfaces/io.github.geopm.xml
    fi
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/geopm.service \
    ${sysconfdir}/dbus-1/system.d/io.github.geopm.conf \
    ${datadir}/dbus-1/interfaces/io.github.geopm.xml \
"

RDEPENDS:${PN} += " ${@bb.utils.contains('DISTRO_FEATURES','systemd','systemd','',d)}"
