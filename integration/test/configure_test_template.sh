#!/bin/bash

#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

#
# Used to add an integration test to the build and create a template
# with boiler plate code.
#

if [ $# -ne 1 ]; then
    echo "Usage: $0 test_name"
    exit -1
fi

if [ $(basename $PWD) != test -o $(basename $(dirname $PWD)) != integration ]; then
    echo "Error: Must be run in the integration/test subdirectory of the GEOPM repository"
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
echo "include integration/test/test_$test_name.mk" >> Makefile.mk

# Create test templates
configure_template test_template.cpp.in test_$test_name.cpp
configure_template test_template.py.in test_$test_name.py
chmod u+x test_$test_name.py

# Add binary to .gitignore
insert_ordered /integration/test/test_$test_name ../../.gitignore
