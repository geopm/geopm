#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

test_name=$(basename $0)
test_dir=$(dirname $0)
obj_dir=$(readlink -f ${test_dir}/../..)
lib_path=${obj_dir}/.libs
real_path=$(dirname $(readlink -f $0))
top_dir=$(readlink -f ${real_path}/..)

test_class=$(echo ${test_name} | awk -F. '{print $1}')
lib_dir=${obj_dir}/.libs
export PYTHONPATH=${top_dir}:${PYTHONPATH}
export LD_LIBRARY_PATH=${lib_dir}:${LD_LIBRARY_PATH}

# PYTHON is expected to be set when running make check. However, the default
# value may be used when invoking this script directly from the command line.
"${PYTHON:-python3}" ${top_dir}/geopmdpy_test/${test_class}.py --verbose ${test_name} >& ${test_dir}/${test_name}.log
