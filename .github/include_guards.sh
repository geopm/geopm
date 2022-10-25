#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

ret=0

echo "Checking include guards in headers."
DIR="src/ test/ service/src/"
for file in $(find $DIR -name "*.h" -o -name "*.hpp"); do
    guard=$(basename $file | sed "s|\.|_|g" | tr 'a-z' 'A-Z')_INCLUDE
    if [ $(grep -c $guard $file) -lt 2 ]; then
        echo "$file has missing or incorrect include guard"
        echo "Expected: $guard"
        ret=1
    fi
done

exit $ret
