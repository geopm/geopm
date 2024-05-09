#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

print_help() {
    echo "
    Usage: $0 VERSION [USER]
           $0 --remove

    Installs the RPMs created by the \"make rpm\" target of the
    geopm/service build.  The geopm-service libgeopmd-devel and
    python3-geopmdpy packages are installed and then the geopm service
    is started with systemctl.

    The geopm-service requires dasbus version 1.5 or later is
    installed.  Use the script build_dasbus.sh located in this
    directory to create the required dasbus RPM if it is not already
    installed on your system.  The script prints instructions on how
    to install the RPM it creates.

    Example:

        sudo ./install_service.sh \$(cat ../VERSION) \$USER

"
    exit 0
}

install_error() {
    echo "Error: $1" 1>&2
    exit -1
}

# Support both yum and zypper
if which zypper >& /dev/null; then
    PKG_INSTALL="zypper --no-remote --non-interactive install --allow-unsigned-rpm"
    PKG_REMOVE="zypper --non-interactive remove"
elif which yum >& /dev/null; then
    PKG_INSTALL="yum install -y"
    PKG_REMOVE="yum remove -y"
else
    PKG_INSTALL="rpm --install"
    PKG_REMOVE="rpm --erase"
fi

install_packages() {
    VERSION=$1
    RPM_USER=$2
    RPM_DIR=/home/${RPM_USER}/rpmbuild/RPMS
    PACKAGES="\
${RPM_DIR}/x86_64/libgeopmd2-${VERSION}-1.x86_64.rpm
${RPM_DIR}/x86_64/python3-geopmdpy-${VERSION}-1.x86_64.rpm
${RPM_DIR}/x86_64/geopm-service-${VERSION}-1.x86_64.rpm"
    for PKG in ${PACKAGES}; do
        test -f ${PKG} ||
            install_error "File does not exist: ${PKG}"
    done
    ${PKG_INSTALL} ${PACKAGES} ||
        install_error "Failed to install the following package: ${PKG}"
}

start_service() {
    systemctl start geopm ||
        install_error "Failed to start the geopm service"
}

remove_service() {
    IS_QUIET=0
    if [[ $# -eq 1 ]] && [[ $1 == '--quiet' ]]; then
	IS_QUIET=1
    fi
    if [[ $(systemctl is-active geopm) == 'active' ]]; then
        systemctl stop geopm ||
            echo "Warning: Failed to stop geopm service" 1>&2
    fi
    for pkg in geopm-service python3-geopmdpy libgeopmd2; do
	if [[ ${IS_QUIET} -eq 0 ]]; then
            ${PKG_REMOVE} $pkg ||
                echo "Warning: Failed to remove geopm service package: $pkg" 1>&2
	else
	    ${PKG_REMOVE} $pkg >& /dev/null
	fi
    done
}

if [[ $# -lt 1 ]] || [[ $1 == '--help' ]]; then
    print_help
elif [[ ${USER} != "root" ]]; then
    install_error "Script must be run as user root"
elif [[ $# -eq 1 ]] && [[ $1 == '--remove' ]]; then
    remove_service
else
    GEOPM_VERSION=$1
    if [[ $# -gt 1 ]]; then
        RPM_USER=$2
    else
        RPM_USER=${USER}
    fi
    remove_service --quiet
    install_packages ${GEOPM_VERSION} ${RPM_USER} &&
    start_service
fi
