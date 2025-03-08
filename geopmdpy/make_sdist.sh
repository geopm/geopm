#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if which protoc >& /dev/null; then
    ./protoc-gen.sh
fi
python3 make_sdist.py | tee make_sdist.log
