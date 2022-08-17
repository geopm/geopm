#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

TEST_DIR=$(dirname "$0")
python3 $TEST_DIR/test_su_term_batch_write_helper.py &
echo $!
