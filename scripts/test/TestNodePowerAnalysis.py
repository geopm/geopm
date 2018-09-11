#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

import unittest
from analysis_helper import *


class NodePowerAnalysis(unittest.TestCase):
    def setUp(self):
        if g_skip_analysis_test:
            self.skipTest(g_skip_analysis_ex)

    def test_node_efficiency_process(self):
        report_df = pandas.DataFrame()
        analysis = geopmpy.analysis.NodePowerAnalysis(**self._config)
        process_output = analysis.plot_process(report_df)
        expected_cols = ['reference_mean', 'reference_max', 'reference_min',
                         'target_mean', 'target_max', 'target_min',
                         'reference_max_delta', 'reference_min_delta',
                         'target_max_delta', 'target_min_delta']
        for ee, rr in zip(expected_cols, process_output.columns):
            self.assertEqual(ee, rr)
        expected_index = self._powers
        for ee, rr in zip(expected_index, process_output.index):
            self.assertEqual(ee, rr)

        # TODO: check that total of all bins adds up to num nodes
