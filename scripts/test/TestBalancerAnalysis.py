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

import os
import unittest
from analysis_helper import *


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestBalancerAnalysis(unittest.TestCase):
    def setUp(self):
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
        # default mocked data for each column given power budget and agent
        self._agent_factor = {
            'power_governor': 1.0,
            'power_balancer': 0.9,
        }
        self._gen_val = {
            'count': (lambda pow, agent: 1),
            'energy_pkg': (lambda pow, agent: (14000.0 + pow) * self._agent_factor[agent]),
            'energy_dram': (lambda pow, agent: 2000.0),
            'frequency': (lambda pow, agent: 1.0e9 + (self._max_power/float(pow))*1.0e9),
            'mpi_runtime': (lambda pow, agent: 10),
            'runtime': (lambda pow, agent: (500.0 * (1.0/pow)) * self._agent_factor[agent]),
            'id': (lambda pow, agent: 'bad'),
            'power': (lambda pow, agent: self._gen_val['energy_pkg'](pow, agent) / self._gen_val['runtime'](pow, agent)),
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
        node_name = 'mynode'
        region_id = {
            'epoch':  '9223372036854775808',
            'dgemm':  '11396693813',
            'stream': '20779751936'
        }
        index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
        numeric_cols = ['count', 'energy_pkg', 'energy_dram', 'frequency', 'mpi_runtime', 'runtime', 'id']
        regions = ['epoch', 'dgemm', 'stream']
        iterations = range(1, 4)
        start_time = 'Tue Nov  6 08:00:00 2018'

        input_data = {}
        for col in numeric_cols:
            input_data[col] = {}
            for pp in self._powers:
                prof_name = '{}_{}'.format(self._name_prefix, pp)
                for it in iterations:
                    for agent in ['power_governor', 'power_balancer']:
                        for region in regions:
                            self._gen_val['id'] = lambda pow, agent: region_id[region]
                            index = (version, start_time, prof_name, agent, node_name, it, region)
                            value = self._gen_val[col](pp, agent)
                            input_data[col][index] = value

        df = pandas.DataFrame.from_dict(input_data)
        df.index.rename(index_names, inplace=True)
        return df

    def make_expected_summary_df(self, metric):
        ref_val_cols = ['reference_mean', 'reference_max', 'reference_min']
        tar_val_cols = ['target_mean', 'target_max', 'target_min']
        delta_cols = ['reference_max_delta', 'reference_min_delta', 'target_max_delta', 'target_min_delta']
        cols = ref_val_cols + tar_val_cols + delta_cols
        expected_data = []
        ref_agent = 'power_governor'
        tar_agent = 'power_balancer'
        for pp in self._powers:
            row = []
            row += [self._gen_val[metric](pp, ref_agent) for col in ref_val_cols]
            row += [self._gen_val[metric](pp, tar_agent) for col in tar_val_cols]
            row += [0.0 for col in delta_cols]
            expected_data.append(row)
        index = pandas.Index(self._powers, name='name')
        return pandas.DataFrame(expected_data, index=index, columns=cols)

    def test_balancer_plot_process_runtime(self):
        metric = 'runtime'
        report_df = self.make_mock_report_df()
        mock_parse_data = MockAppOutput(report_df)
        analysis = geopmpy.analysis.BalancerAnalysis(metric=metric, normalize=False, speedup=False,
                                                     **self._config)
        result = analysis.plot_process(mock_parse_data)
        expected_df = self.make_expected_summary_df(metric)

        compare_dataframe(self, expected_df, result)

    def test_balancer_plot_process_energy(self):
        report_df = self.make_mock_report_df()
        mock_parse_data = MockAppOutput(report_df)
        analysis = geopmpy.analysis.BalancerAnalysis(metric='energy', normalize=False, speedup=False,
                                                     **self._config)
        result = analysis.plot_process(mock_parse_data)
        expected_df = self.make_expected_summary_df('energy_pkg')

        compare_dataframe(self, expected_df, result)

    def test_balancer_plot_process_power(self):
        metric = 'power'
        report_df = self.make_mock_report_df()
        mock_parse_data = MockAppOutput(report_df)
        analysis = geopmpy.analysis.BalancerAnalysis(metric=metric, normalize=False, speedup=False,
                                                     **self._config)
        result = analysis.plot_process(mock_parse_data)
        expected_df = self.make_expected_summary_df(metric)

        compare_dataframe(self, expected_df, result)


if __name__ == '__main__':
    unittest.main()
