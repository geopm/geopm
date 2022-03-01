#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
