#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -e
set -x

TMP_FILE=$(mktemp)
geopmsession --help > $TMP_FILE
test -s $TMP_FILE
rm -f $TMP_FILE

TMP_FILE=$(mktemp)
geopmsession --version > $TMP_FILE
test -s $TMP_FILE
rm -f $TMP_FILE
