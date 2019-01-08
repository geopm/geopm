#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

import sys

try:
    import pandas
    import geopm_context
    import geopmpy.analysis
    import geopmpy.io
    g_skip_analysis_test = False
    g_skip_analysis_ex = None
except ImportError as ex:
    g_skip_analysis_test = True
    g_skip_analysis_ex = "Warning, analysis and plotting requires several modules to be installed: {}".format(ex)


def compare_dataframe(test, expected_df, result):
    expected_columns = expected_df.columns
    expected_index = expected_df.index
    result_columns = result.columns
    result_index = result.index
    test.assertEqual(len(expected_columns), len(result_columns))
    test.assertEqual(len(expected_index), len(result_index))
    test.assertTrue((expected_columns == result_columns).all())
    test.assertTrue((expected_index == result_index).all())
    try:
        pandas.testing.assert_frame_equal(expected_df, result)
    except AssertionError:
        sys.stderr.write('\nResult:\n')
        sys.stderr.write(result.to_string())
        sys.stderr.write('\nExpected:\n')
        sys.stderr.write(expected_df.to_string())
        pandas.testing.assert_frame_equal(expected_df, result)


if not g_skip_analysis_test:
    class MockAppOutput(geopmpy.io.AppOutput):
        def __init__(self, report_df):
            self._reports_df = report_df
