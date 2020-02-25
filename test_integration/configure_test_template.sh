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

#
# Used to add an integration test to the build and create a template
# with boiler plate code.
#

if [ $# -ne 1 ]; then
    echo "Usage: $0 test_name"
    exit -1
fi

if [ $(basename $PWD) != test_integration ]; then
    echo "Error: Must be run in the test_integration subdirectory of the GEOPM repository"
    exit -1
fi

test_name=$1
TEST_NAME=${test_name^^}

function insert_ordered
{
    line=$1
    file=$2
    echo $line >> $file
    sort $file -o $file
}

function configure_template
{
    sed -e "s|@test_name@|$test_name|g" -e "s|@TEST_NAME@|$TEST_NAME|g" < $1 > $2
}

# Update makefile
configure_template test_template.mk.in test_$test_name.mk
echo "include test_integration/test_$test_name.mk" >> Makefile.mk

# Create test templates
configure_template test_template.cpp.in test_$test_name.cpp
configure_template test_template.py.in test_$test_name.py
chmod u+x test_$test_name.py

# Add binary to .gitignore
insert_ordered /test_integration/test_$test_name ../.gitignore
