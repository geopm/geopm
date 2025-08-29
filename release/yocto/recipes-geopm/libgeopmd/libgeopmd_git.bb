# Copyright (c) 2015 - 2025 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause

SUMMARY = "GEOPM - libgeopmd shared library (service IO, telemetry, controls)"
HOMEPAGE = "https://geopm.github.io"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE-BSD-3-Clause;md5=d36f2aa7fd5b17cb662c664d6bc3f870"

PV = "3.2.0"
SRCREV = "68f4ccce210f63b24d941b6373de29b839cf3711"
SRC_URI = "git://github.com/geopm/geopm;branch=refs/tags/v3.2.0;protocol=https"
S = "${WORKDIR}/git/libgeopmd"

inherit autotools pkgconfig

# Minimal feature set suitable for BMC/Yocto; enable systemd sd-bus, disable GPU/MSR/x86 specifics
PACKAGECONFIG ??= "systemd"
PACKAGECONFIG[systemd] = "--enable-systemd,--disable-systemd,systemd"
PACKAGECONFIG[grpc] = "--enable-grpc,--disable-grpc,grpc protobuf"
PACKAGECONFIG[libcap] = "--enable-libcap,--disable-libcap,libcap"
PACKAGECONFIG[io-uring] = "--enable-io-uring,--disable-io-uring,liburing"

EXTRA_OECONF = " \
    --disable-nvml \
    --disable-dcgm \
    --disable-levelzero \
    --disable-rawmsr \
    --disable-cpuid \
    --disable-cnl-iogroup \
"

DEPENDS += " \
    zlib \
    ${@bb.utils.contains('PACKAGECONFIG', 'systemd', 'systemd', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'io-uring', 'liburing', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'grpc', 'grpc protobuf', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'libcap', 'libcap', '', d)} \
"

FILES:${PN} += "${libdir}/libgeopmd.so.*"
FILES:${PN}-dev += "${includedir} ${libdir}/libgeopmd.so ${libdir}/pkgconfig/*.pc"

RRECOMMENDS:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'systemd', 'systemd', '', d)}"

# The project uses non-fatal -Og in debug and LTO by default; let upstream flags stand.
