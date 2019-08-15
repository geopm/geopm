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

from builtins import range
from past.utils import old_div
import os
import unittest
from test.analysis_helper import *
from test import mock_report


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestBalancerAnalysis(unittest.TestCase):
    def setUp(self):
        self._name_prefix = 'prof'
        self._use_agent = True
        self._min_power = 160
        self._max_power = 200
        self._step_power = 10
        self._powers = list(range(self._min_power, self._max_power+self._step_power, self._step_power))
        self._node_names = ['mynode']
        self._config = {'profile_prefix': self._name_prefix,
                        'output_dir': '.',
                        'verbose': True,
                        'iterations': 1,
                        'min_power': self._min_power, 'max_power': self._max_power,
                        'step_power': self._step_power}
        self._tmp_files = []
        # default mocked data for each column given power budget and agent
        self._gen_val_governor = {
            'count': (lambda node, region, pow: 1),
            'energy_pkg': (lambda node, region, pow: (14000.0 + pow)),
            'energy_dram': (lambda node, region, pow: 2000.0),
            'frequency': (lambda node, region, pow: 1.0e9 + (self._max_power/float(pow))*1.0e9),
            'mpi_runtime': (lambda node, region, pow: 10),
            'runtime': (lambda node, region, pow: (500.0 * (1.0/pow))),
            'id': (lambda node, region, pow: 'bad'),
            'power': (lambda node, region, pow:
                old_div(self._gen_val_governor['energy_pkg'](node, region, pow),
                self._gen_val_governor['runtime'](node, region, pow))),
        }
        self._gen_val_balancer = self._gen_val_governor.copy()
        for metric in ['energy_pkg', 'runtime']:
            self._gen_val_balancer[metric] = lambda node, region, power: (
                self._gen_val_governor[metric](node, region, power) * 0.9)
        self._gen_val_balancer['power'] = lambda node, region, power: (
                old_div(self._gen_val_balancer['energy_pkg'](node, region, power),
                self._gen_val_balancer['runtime'](node, region, power)) )

        self._agent_params = {
                'power_governor': (self._gen_val_governor, self._powers),
                'power_balancer': (self._gen_val_balancer, self._powers) }

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def make_expected_summary_df(self, metric):
        ref_val_cols = ['reference_mean', 'reference_max', 'reference_min']
        tar_val_cols = ['target_mean', 'target_max', 'target_min']
        delta_cols = ['reference_max_delta', 'reference_min_delta', 'target_max_delta', 'target_min_delta']
        cols = ref_val_cols + tar_val_cols + delta_cols
        expected_data = []
        for pp in self._powers:
            row = []
            # Reference metrics
            row += [self._gen_val_governor[metric](None, None, pp) for col in ref_val_cols]
            # Target metrics
            row += [self._gen_val_balancer[metric](None, None, pp) for col in tar_val_cols]
            row += [0.0 for col in delta_cols]
            expected_data.append(row)
        index = pandas.Index(self._powers, name='name')
        return pandas.DataFrame(expected_data, index=index, columns=cols)

    def test_balancer_plot_process_runtime(self):
        metric = 'runtime'
        report_df = mock_report.make_mock_report_df(
                self._name_prefix, self._node_names, self._agent_params)
        mock_parse_data = MockAppOutput(report_df)
        analysis = geopmpy.analysis.BalancerAnalysis(metric=metric, normalize=False, speedup=False,
                                                     **self._config)
        result = analysis.plot_process(mock_parse_data)
        expected_df = self.make_expected_summary_df(metric)

        compare_dataframe(self, expected_df, result)

    def test_balancer_plot_process_energy(self):
        report_df = mock_report.make_mock_report_df(
                self._name_prefix, self._node_names, self._agent_params)
        mock_parse_data = MockAppOutput(report_df)
        analysis = geopmpy.analysis.BalancerAnalysis(metric='energy', normalize=False, speedup=False,
                                                     **self._config)
        result = analysis.plot_process(mock_parse_data)
        expected_df = self.make_expected_summary_df('energy_pkg')

        compare_dataframe(self, expected_df, result)

    def test_balancer_plot_process_power(self):
        metric = 'power'
        report_df = mock_report.make_mock_report_df(
                self._name_prefix, self._node_names, self._agent_params)
        mock_parse_data = MockAppOutput(report_df)
        analysis = geopmpy.analysis.BalancerAnalysis(metric=metric, normalize=False, speedup=False,
                                                     **self._config)
        result = analysis.plot_process(mock_parse_data)
        expected_df = self.make_expected_summary_df(metric)

        compare_dataframe(self, expected_df, result)


if __name__ == '__main__':
    unittest.main()
