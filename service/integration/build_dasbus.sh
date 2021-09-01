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
    RPM_DIR=/home/${USER}/rpmbuild/RPMS
    PACKAGES="$(ls ${RPM_DIR}/noarch/python3-dasbus-1.6*.noarch.rpm)"
    echo
    echo "Run the following command as root to install the packages:"
    echo "    ${PKG_INSTALL} ${PACKAGES}"
    echo
}

build_dasbus() {
   mkdir -p $HOME/rpmbuild/SOURCES &&
   mkdir -p $HOME/rpmbuild/SPECS &&
   wget https://github.com/rhinstaller/dasbus/releases/download/v1.6/dasbus-1.6.tar.gz \
       -O $HOME/rpmbuild/SOURCES/dasbus-1.6.tar.gz &&
   wget https://raw.githubusercontent.com/rhinstaller/dasbus/v1.6/python-dasbus.spec \
       -O $HOME/rpmbuild/SPECS/python-dasbus.spec &&
   rpmbuild -ba $HOME/rpmbuild/SPECS/python-dasbus.spec &&
   echo '[SUCCESS]' ||
   install_error "Unable to build dasbus package"
}

if [[ $1 == '--help' ]]; then
    print_help
else
    build_dasbus
    print_install_command
fi
