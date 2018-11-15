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


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestNodeEfficiencyAnalysis(unittest.TestCase):
    def setUp(self):
        self._name_prefix = 'prof'
        self._use_agent = True
        self._min_power = 160
        self._max_power = 200
        self._step_power = 10
        self._powers = range(self._min_power, self._max_power+self._step_power, self._step_power)
        self._sticker_freq = 1.5e9
        self._config = {'profile_prefix': self._name_prefix,
                        'output_dir': '.',
                        'verbose': True,
                        'iterations': 1,
                        'min_power': self._min_power, 'max_power': self._max_power,
                        'step_power': self._step_power,
                        'min_freq': 0.5e9, 'max_freq': 2.0e9,
                        'step_freq': 0.1e9, 'sticker_freq': self._sticker_freq,
                        'nodelist': None,
                        }
        self._tmp_files = []
        self._num_nodes = 8
        # default mocked data for each column given node id
        self._gen_val = {
            'count': (lambda node, pow: 1),
            'energy_pkg': (lambda node, pow: 14000.0),
            'energy_dram': (lambda node, pow: 2000.0),
            'frequency': (lambda node, pow: 50.0 + (float(pow)/self._max_power)*(float(node)/self._num_nodes)),
            'mpi_runtime': (lambda node, pow: 10),
            'runtime': (lambda node, pow: 50.0),
            'id': (lambda node, pow: 'bad'),
        }

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def make_mock_report_df(self):
        # for input data frame
        version = '0.3.0'
        region_id = {
            'epoch':  '9223372036854775808',
            'dgemm':  '11396693813',
            'stream': '20779751936'
        }
        start_time = 'Tue Nov  6 08:00:00 2018'
        index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
        numeric_cols = ['count', 'energy_pkg', 'energy_dram', 'frequency', 'mpi_runtime', 'runtime', 'id']
        iterations = range(1, 4)
        input_data = {}
        for col in numeric_cols:
            input_data[col] = {}
            for agent in ['power_balancer', 'power_governor']:
                for power_cap in self._powers:
                    prof_name = '{}_{}'.format(self._name_prefix, power_cap)
                    for node in range(self._num_nodes):
                        node_name = 'node{}'.format(node)
                        for it in iterations:
                            for region in region_id.keys():
                                self._gen_val['id'] = lambda node, power_cap: region_id[region]
                                index = (version, start_time, prof_name, agent, node_name, it, region)
                                value = self._gen_val[col](node, power_cap)
                                input_data[col][index] = value

        df = pandas.DataFrame.from_dict(input_data)
        df.index.rename(index_names, inplace=True)
        return df

    def make_expected_summary_df(self, power_cap, agent):
        cols = ['frequency']
        expected_data = []
        for node in range(self._num_nodes):
            val = self._gen_val['frequency'](node, power_cap)
            val *= 0.01 * self._sticker_freq / 1e9
            expected_data.append(val)
        nodes = ['node{}'.format(n) for n in range(self._num_nodes)]
        index = pandas.Index(nodes, name='node_name')
        return pandas.DataFrame(expected_data, index=index, columns=cols)

    def test_node_efficiency_process(self):
        analysis = geopmpy.analysis.NodeEfficiencyAnalysis(**self._config)
        report_df = self.make_mock_report_df()
        mock_parse_data = MockAppOutput(report_df)
        gov_result, bal_result = analysis.plot_process(mock_parse_data)
        for pow in self._powers:
            self.assertEqual(self._num_nodes, len(gov_result[pow]))
            self.assertEqual(self._num_nodes, len(bal_result[pow]))
            for agent in ['power_governor', 'power_balancer']:
                gov_expected_df = self.make_expected_summary_df(pow, 'power_governor')
                bal_expected_df = self.make_expected_summary_df(pow, 'power_balancer')
                compare_dataframe(self, gov_expected_df, gov_result[pow])
                compare_dataframe(self, bal_expected_df, bal_result[pow])


if __name__ == '__main__':
    unittest.main()
