#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Creates all of the RPM or Debian packages in the repository

# Set GEOPM_PACKGE_SKIP_RUNTIME to build only the GEOPM Access Service packages.
#
# All command line arguments are forwarded to the configure calls used to create
# source archives.

if grep -i ubuntu /etc/os-release || grep -i debian /etc/os-release; then
    pkg=deb
else
    pkg=rpm
fi

set -e

cd libgeopmd
./autogen.sh
./configure $@
make $pkg
cd -

cd geopmdpy
./make_$pkg.sh
cd -

if which cargo >& /dev/null; then
    cd geopmdrs
    ./build.sh
cd -
else
    echo "Install rust to enable grpc features" 1>&2
fi

cd docs
./make_$pkg.sh
cd -

if [ -z "$GEOPM_PACKAGE_SKIP_RUNTIME" ]; then

    cd libgeopm
    ./autogen.sh
    ./configure $@ --disable-mpi --disable-openmp
    make $pkg
    cd -

    cd geopmpy
    ./make_$pkg.sh
    cd -
fi
