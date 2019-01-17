#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#https://d-meiser.github.io/2016/01/10/mpi-travis.html

make
if [ $CI_MODE == "unit" ]; then
    make check && ./copying_headers/test-dist
    exit $?
elif [ $CI_MODE == "integration" ]; then
    if [ -f mpich/lib/libmpich.so ]; then
        echo "libmpich.so found -- nothing to build."
    else
        echo "Downloading mpich source."
        wget http://www.mpich.org/static/downloads/3.2/mpich-3.2.tar.gz
        tar xfz mpich-3.2.tar.gz
        rm mpich-3.2.tar.gz
        echo "configuring and building mpich."
        cd mpich-3.2
        ./configure \
            --prefix=`pwd`/../mpich \
            --enable-static=false \
            --enable-alloca=true \
            --disable-long-double \
            --enable-threads=single \
            --enable-fortran=no \
            --enable-fast=all \
            --enable-g=none \
            --enable-timing=none
        make
        make install
        cd -
        rm -rf mpich-3.2
    fi
    make install && make dist && cd test_integration
    #todo need to specify mpich mpirun targeting launcher
    GEOPM_KEEP_FILES=barrelroll GEOPM_RUN_LONG_TESTS=yup ./geopm_test_integration.py -v TestIntegration.test_count
    exit $?
else
    echo "Unsupported CI_MODE."
    exit 1
fi
