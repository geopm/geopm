#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

print_help() {
    echo "
    The geopm-service requires dasbus version 1.5 or later.  This
    script will download dasbus version 1.6 and build the RPM from the
    source published to the rhinstaller/dasbus github site.

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
elif which zypper >& /dev/null; then
    PKG_INSTALL="zypper install -y --allow-unsigned-rpm"
else
    PKG_INSTALL="yum install -y"
fi

print_install_command() {
    local RPM_DIR=$(rpm --eval "%_rpmdir")
    PACKAGES="$(ls ${RPM_DIR}/noarch/python3-dasbus-1.6*.noarch.rpm)"
    echo
    echo "Run the following command as root to install the packages:"
    echo "    ${PKG_INSTALL} ${PACKAGES}"
    echo
}

build_dasbus() {
   local RPM_SOURCEDIR=$(rpm --eval "%_sourcedir")
   local RPM_SPECDIR=$(rpm --eval "%_specdir")

   mkdir -p $RPM_SOURCEDIR &&
   mkdir -p $RPM_SPECDIR &&
   wget https://github.com/rhinstaller/dasbus/releases/download/v1.6/dasbus-1.6.tar.gz \
       -O $RPM_SOURCEDIR/dasbus-1.6.tar.gz &&
   wget https://raw.githubusercontent.com/rhinstaller/dasbus/v1.6/python-dasbus.spec \
       -O $RPM_SPECDIR/python-dasbus.spec &&
   rpmbuild -ba $RPM_SPECDIR/python-dasbus.spec &&
   echo '[SUCCESS]' ||
   install_error "Unable to build dasbus package"
}

if [[ $1 == '--help' ]]; then
    print_help
else
    build_dasbus
    print_install_command
fi
