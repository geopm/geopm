#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x
# Create VERSION file
if [ ! -e VERSION ]; then
    python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > VERSION
    if [ $? -ne 0 ]; then
        echo "WARNING:  VERSION file does not exist and setuptools_scm failed, setting version to 0.0.0" 1>&2
        echo "0.0.0" > VERSION
    fi
fi
autoreconf -i -f
