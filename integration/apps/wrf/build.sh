#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

set -x
set -e

# Get helper functions
source ../build_func.sh

get_netcdf_source() {
    ARCHIVE=netcdf-4.1.3.tar.gz
    URL=http://www2.mmm.ucar.edu/wrf/OnLineTutorial/compile_tutorial/tar_files/
    get_archive ${ARCHIVE} ${URL}
    tar xzvf ${ARCHIVE}
    cd netcdf-4.1.3
    ./configure --prefix=$DIR/netcdf --disable-dap --disable-netcdf-4 --disable-shared
    make
    make install
    export PATH=$DIR/netcdf/bin:$PATH
    export NETCDF=$DIR/netcdf
    cd ..
}

get_zlib_source() {
    ARCHIVE=zlib-1.2.7.tar.gz
    URL=http://www2.mmm.ucar.edu/wrf/OnLineTutorial/compile_tutorial/tar_files/
    get_archive ${ARCHIVE} ${URL}
    tar xzvf ${ARCHIVE}
    cd zlib-1.2.7
    ./configure --prefix=$DIR/grib2
    make
    make install
    cd ..
}

get_libpng_source() {
    ARCHIVE=libpng-1.2.50.tar.gz
    URL=http://www2.mmm.ucar.edu/wrf/OnLineTutorial/compile_tutorial/tar_files/
    get_archive ${ARCHIVE} ${URL}
    tar xzvf ${ARCHIVE}
    cd libpng-1.2.50
    ./configure --prefix=$DIR/grib2
    make
    make install
    cd ..
}

get_jasper_source() {
    ARCHIVE=jasper-1.900.1.tar.gz
    URL=http://www2.mmm.ucar.edu/wrf/OnLineTutorial/compile_tutorial/tar_files/
    get_archive ${ARCHIVE} ${URL}
    tar xzvf ${ARCHIVE}
    cd jasper-1.900.1
    ./configure --prefix=$DIR/grib2
    make
    make install
    cd ..
}

get_wrf_source() {
    local DIRNAME=${1}
    ARCHIVE=WRFV3.9.1.1.TAR.gz
    URL=http://www2.mmm.ucar.edu/wrf/src/
    get_archive ${ARCHIVE} ${URL}
    tar xzvf ${ARCHIVE}
}

get_conus2_5km_source() {
    ARCHIVE=bench_2.5km.tar.bz2
    URL=https://www2.mmm.ucar.edu/wrf/bench/conus2.5km_v3911/
    get_archive ${ARCHIVE} ${URL}
    tar xvf ${ARCHIVE}
}

# Setup env variables for workload
export DIR=${GEOPM_SOURCE}/integration/apps/wrf/LIBRARIES
export NETCDF=$DIR/netcdf
export JASPERLIB=$DIR/grib2/lib
export JASPERINC=$DIR/grib2/include
export LDFLAGS=-L$DIR/grib2/lib
export CPPFLAGS=-I$DIR/grib2/include
export PATH=$DIR/netcdf/bin:$PATH
export WRFIO_NCD_LARGE_FILE_SUPPORT=1

# Acquire the dependencies:
get_zlib_source
get_libpng_source
get_jasper_source
get_netcdf_source

#Get inputs for run
get_conus2_5km_source

#Get the workload
DIRNAME=WRFV3
clean_source ${DIRNAME}
get_wrf_source ${DIRNAME}

#Setup git and apply patches
setup_source_git ${DIRNAME}

# Run configure
cd ${DIRNAME}
./configure <<'EOF'
67
1
EOF
cd ../

cd ${DIRNAME}

#Finally build WRF
./compile -j 20 wrf >& wrf_compile.log

#Put inputs in run dir
ln -s ${GEOPM_SOURCE}/integration/apps/wrf/bench_2.5km/diffwrf.py ${GEOPM_SOURCE}/integration/apps/wrf/${DIRNAME}/run/
ln -s ${GEOPM_SOURCE}/integration/apps/wrf/bench_2.5km/namelist.input ${GEOPM_SOURCE}/integration/apps/wrf/${DIRNAME}/run/
ln -s ${GEOPM_SOURCE}/integration/apps/wrf/bench_2.5km/wrfbdy_d01 ${GEOPM_SOURCE}/integration/apps/wrf/${DIRNAME}/run/
ln -s ${GEOPM_SOURCE}/integration/apps/wrf/bench_2.5km/wrfout_d01_2005-06-04_12:00:00 ${GEOPM_SOURCE}/integration/apps/wrf/${DIRNAME}/run/
ln -s ${GEOPM_SOURCE}/integration/apps/wrf/bench_2.5km/wrfrst_d01_2005-06-04_09:00:00 ${GEOPM_SOURCE}/integration/apps/wrf/${DIRNAME}/run/
