#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

test_dir=$(dirname $0)
tests=`ls ${test_dir}/Test*.py`

for t in $tests; do
    ${t} --verbose >& ${t}.log
done
