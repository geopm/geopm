#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

TEST_DIR=$(dirname "$0")
python3 $TEST_DIR/batch_write_client_helper.py &
echo $!
