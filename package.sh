#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Creates all of the RPM or Debian packages in the repository

if grep -i ubuntu /etc/os-release || grep -i debian /etc/os-release; then
    pkg=deb
else
    pkg=rpm
fi

set -e

for cdir in libgeopmd libgeopm; do
    cd $cdir
    ./autogen.sh
    ./configure --disable-mpi --disable-openmp
    make $pkg
    cd -
done

for pdir in geopmdpy geopmpy docs; do
    cd $pdir
    ./make_$pkg.sh
    cd -
done
