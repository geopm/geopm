#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -e
set -x
LIBGEOPMD_PATH=$(realpath $PWD/../libgeopmd)
export CGO_CFLAGS="-I${LIBGEOPMD_PATH}/include"
export CGO_LDFLAGS="-L${LIBGEOPMD_PATH}/.libs"
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBGEOPMD_PATH}/.libs
for TEST_FILE in geopmdgo/*_test.go; do
    SOURCE_FILE=$(echo $TEST_FILE | sed 's|_test\.go$|.go|')
    if [[ ${TEST_FILE} == geopmdgo/error_test.go ]]; then
        go test ${TEST_FILE} ${SOURCE_FILE}
    else
        go test ${TEST_FILE} ${SOURCE_FILE} geopmdgo/error.go
    fi
done

go build examples/geopmread.go
TEMP_FILE=$(mktemp)
./geopmread --help |& tee ${TEMP_FILE}
test $(cat ${TEMP_FILE} | wc -l) -gt 0
./geopmread > ${TEMP_FILE}
test $(cat ${TEMP_FILE} | wc -l) -gt 0
./geopmread TIME board 0 | tee ${TEMP_FILE}
test $(cat ${TEMP_FILE} | wc -l) -gt 0
rm -f ${TEMP_FILE}
