#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Run the test controller
    -----------------------

    This test shows the geopmpy.runtime module being used to run the
    sleep command.

"
    exit 0
fi

test_error() {
    echo "Error: $1" 1>&2
    exit -1
}

TEST_DIR=$(dirname $(readlink -f $0))
TEST_SCRIPT=${TEST_DIR}/test_controller.py

${TEST_SCRIPT} 2.0e9 sleep 5 ||
    test_error "Call to test controller running sleep returned non-zero exit code"

echo "SUCCESS"
exit 0
