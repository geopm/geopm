#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -e
dir_name=$(dirname $0)
cd $dir_name
if [ -d ".git" ]; then
    ./copying_headers/test-license
else
    ./copying_headers/test-license --ignore-service
fi
