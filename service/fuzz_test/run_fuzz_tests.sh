#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

err=0

test_dir=$(dirname $0)

for t in ${test_dir}/*_reg_test; do
    ${t} ${t/reg_test/corpus}/* >& ${t}.log
    result=$?
    if [ $result -ne 0 ]; then
        err=1
    fi
done

exit ${err}
