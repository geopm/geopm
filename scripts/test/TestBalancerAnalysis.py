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

# TODO: profile name should affect performance. it can hide bugs if all the numbers are the same
# however the functions that generate expected output need to also take this into account
def make_mock_sweep_report_df(name_prefix, powers, metric, metric_perf):
    ''' Make a mock report dataframe for the power sweeps.'''
    version = '0.3.0'
    power_budget = 400
    tree_decider = 'static'
    leaf_decider = 'simple'
    node_name = 'mynode'
    region_id = {
        'epoch':  '9223372036854775808',
        'dgemm':  '11396693813',
        'stream': '20779751936'
    }

    # for input data frame
    index_names = ['version', 'name', 'power_budget', 'tree_decider',
                   'leaf_decider', 'agent', 'node_name', 'iteration', 'region']
    numeric_cols = ['count', 'energy_pkg', 'frequency', 'mpi_runtime', 'runtime', 'id']

    # default mocked data for each column
    gen_val = {
        'count': 1,
        'energy_pkg': 14000.0,
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
            prof_name = '{}_{}'.format(name_prefix, pp)
            for it in iterations:
                for agent in ['power_governor', 'power_balancer']:
                    for region in regions:
                        gen_val['id'] = region_id[region]  # return unique region id
                        index = (version, prof_name, power_budget, tree_decider,
                                 leaf_decider, agent, node_name, it, region)
                        value = gen_val[col]
                        if metric == col and region == 'epoch':
                            value = metric_perf[agent][pp] + it
                        input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df


class TestBalancerAnalysis(unittest.TestCase):
    if g_skip_analysis_test:
        self.skipTest(g_skip_analysis_ex)

    def test_balancer_plot_process_runtime(self):
        metric = 'runtime'
        report_df = make_mock_sweep_report_df(self._name_prefix, self._powers,
                                              metric,
                                              {'power_governor': {160: 100,
                                                                  170: 90,
                                                                  180: 80,
                                                                  190: 75,
                                                                  200: 70},
                                               'power_balancer': {160: 91,
                                                                  170: 82,
                                                                  180: 73,
                                                                  190: 70,
                                                                  200: 70}})
        analysis = geopmpy.analysis.BalancerAnalysis(metric=metric, **self._config)
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

        for name in process_output.index:
            row = process_output.loc[name]
            if name == self._max_power:
                # normalized perf to this power setting
                self.assertEqual(1.0, row['reference_mean'])
            if name == 200:
                # normalized perf is the same for both
                self.assertEqual(row['reference_mean'], row['target_mean'])
            else:
                self.assertGreater(row['reference_mean'], row['target_mean'])

    # TODO: test balancer with other metrics

    def test_balancer_plot_process_energy(self):
        pass

    def test_balancer_plot_process_power(self):
        pass
