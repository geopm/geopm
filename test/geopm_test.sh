#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

unset GEOPM_PMPI_CTL
unset GEOPM_POLICY
unset GEOPM_REPORT
unset GEOPM_TRACE

test_name=`basename $0`
dir_name=`dirname $0`
run_test=true
xml_dir=$dir_name
base_dir=$dir_name/../..

export LD_LIBRARY_PATH=$base_dir/.libs:$base_dir/openmp/lib:$LD_LIBRARY_PATH

if [[ $GTEST_XML_DIR ]]; then
    xml_dir=$GTEST_XML_DIR
fi
err=0

# Check for crc32 intrinsic support before running ProfileTable tests
if [[ $test_name =~ ^ProfileTable ]]; then
    if  ! ./examples/geopm_platform_supported crc32; then
        echo "Warning: _mm_crc32_u64 intrisic not supported."
        run_test=false
    fi
fi

# Skipped on Mac because the implementation is lax about invalid shmem construction
if [[ $test_name == SharedMemoryTest.invalid_construction ]]; then
    if  [[ $(uname) == Darwin ]]; then
        run_test=false
    fi
fi

if [ "$run_test" == "true" ]; then
    exec_name=geopm_test
    if [[ $test_name =~ ^MPIInterface ]]; then
        exec_name=geopm_mpi_test_api
    fi
    $dir_name/../.libs/$exec_name \
        --gtest_filter=$test_name --gtest_output=xml:$xml_dir/$test_name.xml >& $dir_name/$test_name.log
    err=$?
fi

if [ "$run_test" != "true" ]; then
    echo "SKIP: $test_name"
else
    # Parse output log to see if the test actually ran.
    if (grep -Fq "[==========] Running 0 tests from 0 test cases." $dir_name/$test_name.log); then
        echo "ERROR: Test $test_name does not exist!"
        err=1
    fi
fi


exit $err
