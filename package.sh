#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Creates all of the RPM or Debian packages in the repository

skip_runtime=0
if [ $# == 2 ] && [ $1 == '--skip-runtime' ]; then
    skip_runtime=1
fi

if grep -i ubuntu /etc/os-release || grep -i debian /etc/os-release; then
    pkg=deb
else
    pkg=rpm
fi

set -e

cd libgeopmd
./autogen.sh
./configure
make $pkg
cd -

cd geopmdpy
./make_$pkg.sh
cd -

cd geopmdrs
./build.sh
cd -

cd docs
./make_$pkg.sh
cd -

if [ $skip_runtime -eq 0 ]; then

    cd libgeopm
    ./autogen.sh
    ./configure --disable-mpi --disable-openmp
    make $pkg
    cd -

    cd geopmpy
    ./make_$pkg.sh
    cd -
fi
