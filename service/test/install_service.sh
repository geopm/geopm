#!/bin/bash
#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

print_help() {
    echo "
    Usage: $0 VERSION [USER]
           $0 --remove

    Installs the RPMs created by the \"make rpm\" target of the geopm/service
    build.  The geopm-service and python3-geopmdpy packages are installed and
    then the geopm service is started with systemctl.  The dasbus python3
    module version 1.5 or later must be installed For the service to start
    successfully.  This can be installed via pip from PyPI or with an RPM
    derived from the dasbus spec file:

    https://github.com/rhinstaller/dasbus/blob/master/python-dasbus.spec

    Note the dasbus spec file is designed to support Fedora packaging.

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
if ! wget -O /dev/null www.google.com >& /dev/null; then
    PKG_INSTALL="rpm --install"
    PKG_REMOVE="rpm --erase"
elif which zypper >& /dev/null; then
    PKG_INSTALL="zypper install -y --allow-unsigned-rpm"
    PKG_REMOVE="zypper remove -y"
else
    PKG_INSTALL="yum install -y"
    PKG_REMOVE="yum remove -y"
fi

install_packages() {
    VERSION=$1
    RPM_USER=$2
    RPM_DIR=/home/${RPM_USER}/rpmbuild/RPMS
    PACKAGES="\
${RPM_DIR}/x86_64/geopm-service-${VERSION}-1.x86_64.rpm
${RPM_DIR}/x86_64/python3-geopmdpy-${VERSION}-1.x86_64.rpm"
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
    systemctl stop geopm ||
        echo "Warning: Failed to stop geopm service" 1>&2
    ${PKG_REMOVE} geopm-service python3-geopmdpy ||
        echo "Warning: Failed to remove geopm service packages" 1>&2
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
    remove_service
    install_packages ${GEOPM_VERSION} ${RPM_USER} &&
    start_service
fi
