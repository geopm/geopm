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

import os
import sys
import unittest
from collections import defaultdict
from StringIO import StringIO
from analysis_helper import *


class TestPowerSweepAnalysis(unittest.TestCase):
    def setUp(self):
        if g_skip_analysis_test:
            self.skipTest(g_skip_analysis_ex)
        self._name_prefix = 'prof'
        self._use_agent = True
        self._min_power = 160
        self._max_power = 200
        self._step_power = 10
        self._powers = range(self._min_power, self._max_power+self._step_power, self._step_power)
        self._config = {'profile_prefix': self._name_prefix,
                        'output_dir': '.',
                        'verbose': True,
                        'iterations': 1,
                        'min_power': self._min_power, 'max_power': self._max_power,
                        'step_power': self._step_power}
        self._tmp_files = []

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def make_mock_report_df(self, powers):
        version = '0.3.0'
        power_budget = 400
        tree_decider = 'static'
        leaf_decider = 'simple'
        agent = 'power_governor'
        node_name = 'mynode'
        region_id = {
            'epoch':  '9223372036854775808',
            'dgemm':  '11396693813',
            'stream': '20779751936'
        }

        # for input data frame
        index_names = ['version', 'name', 'power_budget', 'tree_decider',
                       'leaf_decider', 'agent', 'node_name', 'iteration', 'region']
        numeric_cols = ['count', 'energy_pkg', 'energy_dram', 'frequency', 'mpi_runtime', 'runtime', 'id']

        # default mocked data for each column
        gen_val = {
            'count': 1,
            'energy_pkg': 14000.0,
            'energy_dram': 2000.0,
            'frequency': 1e9,
            'mpi_runtime': 10,
            'runtime': 50,
            'id': 'bad'
        }
        regions = ['epoch', 'dgemm', 'stream']
        iterations = range(1, 4)

        input_data = {}
        for col in numeric_cols:
            input_data[col] = {}
            for pp in powers:
                prof_name = '{}_{}'.format(self._name_prefix, pp)
                for it in iterations:
                    for region in regions:
                        gen_val['id'] = region_id[region]  # return unique region id
                        index = (version, prof_name, power_budget, tree_decider,
                                 leaf_decider, agent, node_name, it, region)
                        value = gen_val[col]
                        #if metric == col and region == 'epoch':
                        #    value = metric_perf[agent][pp] + it
                        input_data[col][index] = value

        df = pandas.DataFrame.from_dict(input_data)
        df.index.rename(index_names, inplace=True)
        return df

    def make_expected_summary_df(self, powers):
        expected = {}
        for pp in powers:
            expected[pp] = [1, 2, 3, 4, 5]
        return pandas.DataFrame.from_dict(expected)

    def test_power_sweep_summary(self):
        powers = range(100, 210, 10)
        sweep_analysis = geopmpy.analysis.PowerSweepAnalysis(**self._config)
        parse_output = self.make_mock_report_df(powers)
        process_output = sweep_analysis.summary_process(parse_output)
        expected_df = self.make_expected_summary_df(powers)
        #self.assertEqual(expected_df, process_output)
        pandas.util.testing.assert_frame_equal(expected_df, process_output)


if __name__ == '__main__':
    unittest.main()
