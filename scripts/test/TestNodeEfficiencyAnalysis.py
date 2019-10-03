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

from __future__ import absolute_import
from __future__ import division

import unittest
from test.analysis_helper import *
from test import mock_report


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestNodeEfficiencyAnalysis(unittest.TestCase):
    def setUp(self):
        self._name_prefix = 'prof'
        self._use_agent = True
        self._min_power = 160
        self._max_power = 200
        self._step_power = 10
        self._powers = list(range(self._min_power, self._max_power+self._step_power, self._step_power))
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
        self._node_names = ['node{}'.format(node) for node in range(self._num_nodes)]
        # default mocked data for each column given node id
        self._gen_val = {
            'count': (lambda node, region, pow: 1),
            'energy_pkg': (lambda node, region, pow: 14000.0),
            'energy_dram': (lambda node, region, pow: 2000.0),
            'frequency': (lambda node, region, pow: 50.0 + (pow/self._max_power)*(self._node_names.index(node)/self._num_nodes)),
            'mpi_runtime': (lambda node, region, pow: 10),
            'runtime': (lambda node, region, pow: 50.0),
            'id': (lambda node, region, pow: 'bad'),
        }

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def make_expected_summary_df(self, power_cap, agent):
        cols = ['frequency']
        expected_data = []
        for node_name in self._node_names:
            val = self._gen_val['frequency'](node_name, None, power_cap)
            val *= 0.01 * self._sticker_freq / 1e9
            expected_data.append(val)
        index = pandas.Index(self._node_names, name='node_name')
        return pandas.DataFrame(expected_data, index=index, columns=cols)

    def test_node_efficiency_process(self):
        analysis = geopmpy.analysis.NodeEfficiencyAnalysis(**self._config)
        report_df = mock_report.make_mock_report_df(
                self._name_prefix, self._node_names,
                {'power_balancer': (self._gen_val, self._powers),
                 'power_governor': (self._gen_val, self._powers)})
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
