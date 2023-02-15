#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

err=0

test_name=$(basename $0)
link_dir=$(dirname $0)
obj_dir=$(readlink -f ${link_dir}/../..)
lib_path=${obj_dir}/.libs
exec_path=${obj_dir}/test/.libs
real_path=$(dirname $(readlink -f $0))
top_dir=$(readlink -f ${real_path}/..)
xml_dir=${link_dir}

if [[ ${GTEST_XML_DIR} ]]; then
    xml_dir=${GTEST_XML_DIR}
fi

export LD_LIBRARY_PATH=${lib_path}:${LD_LIBRARY_PATH}

exec_name=geopm_test
log_file=${link_dir}/${test_name}.log
${exec_path}/${exec_name} \
    --gtest_filter=${test_name} \
    --gtest_output=xml:${xml_dir}/${test_name}.xml >& ${log_file}
err=$?

# Parse output log to see if the test actually ran.
if (grep -Fq "[==========] Running 0 tests" ${log_file}); then
    echo "ERROR: Test ${test_name} does not exist!"
    err=1
fi

exit ${err}
