#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

test_dir=$(dirname $0)
real_path=$(dirname $(readlink -f $0))
top_dir=$(readlink -f ${real_path}/../..)

tests=`ls ${test_dir}/Test*.py`

export PYTHONPATH=${top_dir}:${PYTHONPATH}

for t in $tests; do
    ${t} --verbose >& ${t}.log
done
