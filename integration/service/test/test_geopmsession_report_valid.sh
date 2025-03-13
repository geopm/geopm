#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

set -e
set -x
TMP_FILE=$(mktemp)
printf "TIME * *" | geopmsession -o /dev/null -r ${TMP_FILE} -t 1 -p 0.1
test -s ${TMP_FILE}
python3 check_geopmsession_report_valid.py ${TMP_FILE}
rm -f ${TMP_FILE}
