#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -e
temp_file=$(mktemp)
geopmread --info-all > $temp_file

python3 test_geopmread_info.py $temp_file

return_code=$?
