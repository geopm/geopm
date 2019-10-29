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

import os
import sys
import unittest
from collections import defaultdict
from test.analysis_helper import *
from test import mock_report


power_budget = 400

ratio_inds = [0, 1, 2, 3, 6, 5, 4]

def get_expected_baseline_output_df(profile_names, column,
                                    baseline_metric_perf, profile_metric_perf):
    ''' Create dataframe of the savings values expected to be produced by baseline analysis.'''
    expected_cols = [column]

    metric_change = defaultdict(float)
    for prof in profile_names:
        baseline_perf = baseline_metric_perf['epoch']
        optimal_perf = profile_metric_perf[prof]['epoch']
        metric_change[prof] = ((baseline_perf - optimal_perf) / baseline_perf) * 100

    # arrange into list of lists for DataFrame constructor
    expected_data = []
    for prof in profile_names:
        row = [metric_change[prof]]
        expected_data.append(row)
    expected_df = pandas.DataFrame(expected_data, index=profile_names,
                                   columns=expected_cols)
    return expected_df


def get_expected_mix_output_df(profile_names, column, baseline_metric_perf,
                               profile_metric_perf):
    ''' Create data frame for one mix ratio (row) to be produced by stream mix analysis.'''

    metric_change = defaultdict(float)
    for prof in profile_names:
        baseline_perf = baseline_metric_perf['epoch']
        optimal_perf = profile_metric_perf[prof]['epoch']
        metric_change[prof] = ((baseline_perf - optimal_perf) / baseline_perf) * 100

    # arrange into list of lists for DataFrame constructor
    expected_data = []
    expected_cols = []
    for prof in profile_names:
        expected_data.append(metric_change[prof])
        if 'online' in prof:
            expected_cols.append('online per-phase')
        elif 'offline' in prof:
            expected_cols.append('offline per-phase')
        elif 'freq' in prof:
            expected_cols.append('offline application')
    expected_df = pandas.DataFrame([expected_data], columns=expected_cols)

    return expected_df


def get_expected_app_freqs(best_fit):
    expected_index = ratio_inds
    expected_cols = ['epoch', 'dgemm', 'stream']
    expected_data = []
    for idx in expected_index:
        row = []
        for col in expected_cols:
            row.append(best_fit[col])
        expected_data.append(row)
    expected_df = pandas.DataFrame(expected_data, index=expected_index,
                                   columns=expected_cols)
    return expected_df


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestFreqSweepAnalysis(unittest.TestCase):
    def setUp(self):
        self._name_prefix = 'prof'
        self._use_agent = True
        self._freqs = [1.2e9, 1.3e9, 1.4e9, 1.5e9, 1.6e9]
        self._min_freq = min(self._freqs)
        self._max_freq = max(self._freqs)
        self._step_freq = 100e6
        self._mid_freq = self._max_freq - self._step_freq*2
        config = {'profile_prefix': self._name_prefix, 'output_dir': '.',
                  'verbose': True, 'iterations': 1,
                  'min_freq': self._min_freq, 'max_freq': self._max_freq,
                  'enable_turbo': True}
        self._sweep_analysis = geopmpy.analysis.FreqSweepAnalysis(**config)
        config['enable_turbo'] = False
        self._offline_analysis = geopmpy.analysis.FrequencyMapBaselineComparisonAnalysis(**config)
        self._online_analysis = geopmpy.analysis.EnergyEfficientAgentAnalysis(**config)
        self._mix_analysis = geopmpy.analysis.StreamDgemmMixAnalysis(**config)
        self._tmp_files = []
        self._node_names = ['mynode']
        self._gen_val = {
            'count': lambda node, region, param: 1,
            'energy_pkg': lambda node, region, param: 14000.0,
            'frequency': lambda node, region, param: 1e9,
            'network_time': lambda node, region, param: 10,
            'runtime': lambda node, region, param: 50,
            'id': lambda node, region, param: 'bad'
        }
        self._agent_params = {'energy_efficient': (self._gen_val, [None])}

        self._best_fit_metric_perf = {'dgemm': 23000.0, 'stream': 34000.0, 'epoch': 45000.0}
        self._offline_metric_perf = {'dgemm': 12345.0, 'stream': 23456.0, 'epoch': 34567.0}
        self._online_metric_perf = {'dgemm': 22345.0, 'stream': 33456.0, 'epoch': 44567.0}
        self._optimal_metric_perf = {'dgemm': 22345.0, 'stream': 33456.0, 'epoch': 44567.0}
        self._baseline_metric_perf = {'dgemm': 23456.0, 'stream': 34567.0, 'epoch': 45678.0}

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def set_runtime_best_fit(self, gen_val, best_fit_freq, best_fit_perf):
        """ Make runtime treat given per-region frequencies as best fits.
        Other frequencies take twice as long.
        """
        gen_val['runtime'] = lambda node, region, freq: (
                best_fit_perf[region] if freq == best_fit_freq[region]
                else best_fit_perf[region] * 2.0)

    def set_energy_pkg_best_fit(self, gen_val, best_fit_freq):
        """ Make energy_pkg treat given per-region frequencies as best fits.
        The sweep's max frequency is treated as the baseline, and the default
        energy_pkg value generator is used otherwise.
        """
        def energy_pkg_gen(node, region, freq):
            if freq == best_fit_freq[region]:
                return self._best_fit_metric_perf[region]
            elif self._max_freq and freq == self._max_freq:
                return self._baseline_metric_perf[region]
            else:
                return self._gen_val['energy_pkg'](node, region, freq)
        gen_val['energy_pkg'] = energy_pkg_gen

    def test_region_freq_map(self):
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}
        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        sweep_gen_val = self._gen_val.copy()
        self.set_runtime_best_fit(sweep_gen_val, best_fit_freq, best_fit_perf)
        sweep_params = {'energy_efficient': (sweep_gen_val, self._freqs)}
        parse_out = mock_report.make_mock_report_df(
                self._name_prefix + '_freq', self._node_names, sweep_params)

        parse_out = MockAppOutput(parse_out)
        result = self._sweep_analysis._region_freq_map(parse_out)

        for region in ['epoch', 'dgemm', 'stream']:
            self.assertEqual(best_fit_freq[region], result[region])

    def test_offline_baseline_comparison_report(self):
        baseline_freq = max(self._freqs)
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}
        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        sweep_gen_val = self._gen_val.copy()
        self.set_runtime_best_fit(sweep_gen_val, best_fit_freq, best_fit_perf)
        self.set_energy_pkg_best_fit(sweep_gen_val, best_fit_freq)
        sweep_params = {'energy_efficient': (sweep_gen_val, self._freqs)}
        sweep_reports = mock_report.make_mock_report_df(
                self._name_prefix + '_freq', self._node_names, sweep_params)

        prof_name = self._name_prefix + '_map'
        self._gen_val['energy_pkg'] = lambda node, region, param: self._optimal_metric_perf[region]
        single_run_report = mock_report.make_mock_report_df(
                prof_name, self._node_names, self._agent_params)
        parse_out = sweep_reports.append(single_run_report)
        parse_out.sort_index(ascending=True, inplace=True)
        parse_out = MockAppOutput(sweep_reports), MockAppOutput(parse_out)

        _, _, energy_result = self._offline_analysis.summary_process(parse_out)
        expected_energy_df = get_expected_baseline_output_df([prof_name], 'energy_pkg',
                                                             self._baseline_metric_perf,
                                                             {prof_name: self._optimal_metric_perf})

        result = energy_result.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
        expected = float(expected_energy_df.loc[prof_name])
        self.assertEqual(expected, result)

    def test_online_baseline_comparison_report(self):
        baseline_freq = max(self._freqs)
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}
        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        sweep_gen_val = self._gen_val.copy()
        self.set_runtime_best_fit(sweep_gen_val, best_fit_freq, best_fit_perf)
        self.set_energy_pkg_best_fit(sweep_gen_val, best_fit_freq)
        sweep_params = {'energy_efficient': (sweep_gen_val, self._freqs)}
        sweep_reports = mock_report.make_mock_report_df(
                self._name_prefix + '_freq', self._node_names, sweep_params)

        prof_name = self._name_prefix + '_efficient'
        self._gen_val['energy_pkg'] = lambda node, region, param: self._optimal_metric_perf[region]
        single_run_report = mock_report.make_mock_report_df(
                prof_name, self._node_names, self._agent_params)
        parse_out = sweep_reports.append(single_run_report)
        parse_out.sort_index(ascending=True, inplace=True)
        parse_out = MockAppOutput(sweep_reports), MockAppOutput(parse_out)

        _, _, energy_result = self._online_analysis.summary_process(parse_out)

        expected_energy_df = get_expected_baseline_output_df([prof_name], 'energy_pkg',
                                                             self._baseline_metric_perf,
                                                             {prof_name: self._optimal_metric_perf})

        result = energy_result.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
        expected = float(expected_energy_df.loc[prof_name])
        self.assertEqual(expected, result)

    def test_stream_dgemm_mix_report(self):
        baseline_freq = self._max_freq
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}
        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        sweep_reports = {}
        offline_reports = {}
        online_reports = {}
        expected_energy_df = pandas.DataFrame()
        for mix_idx in ratio_inds:
            name = self._name_prefix + '_mix_{}'.format(mix_idx)
            self._tmp_files.append(name + '_app.config')

            sweep_gen_val = self._gen_val.copy()
            self.set_runtime_best_fit(sweep_gen_val, best_fit_freq, best_fit_perf)
            self.set_energy_pkg_best_fit(sweep_gen_val, best_fit_freq)
            sweep_params = {'energy_efficient': (sweep_gen_val, self._freqs)}
            sweep_df = mock_report.make_mock_report_df(
                    name + '_freq', self._node_names, sweep_params)
            sweep_reports[mix_idx] = MockAppOutput(sweep_df)
            self._gen_val['energy_pkg'] = lambda node, region, param: self._offline_metric_perf[region]
            offline_df = mock_report.make_mock_report_df(
                    name+'_offline', self._node_names, self._agent_params)
            offline_reports[mix_idx] = MockAppOutput(offline_df)
            self._gen_val['energy_pkg'] = lambda node, region, param: self._online_metric_perf[region]
            online_df = mock_report.make_mock_report_df(
                    name+'_online', self._node_names, self._agent_params)
            online_reports[mix_idx] = MockAppOutput(online_df)

            profiles = ['freq_'+str(best_fit_freq['epoch']), 'offline', 'online']
            perfs = [self._best_fit_metric_perf, self._offline_metric_perf, self._online_metric_perf]
            metric_perfs = {}

            profile_names = []
            for ii, prof in enumerate(profiles):
                profile_name = '{}_{}'.format(name, prof)
                profile_names.append(profile_name)
                metric_perfs[profile_name] = perfs[ii]

            energy_df = get_expected_mix_output_df(profile_names, 'energy_pkg',
                                                   self._baseline_metric_perf,
                                                   metric_perfs)
            expected_energy_df = expected_energy_df.append(energy_df, ignore_index=True)

        parse_output = sweep_reports, offline_reports, online_reports
        energy_result, runtime_result, app_freq_df = self._mix_analysis.summary_process(parse_output)

        expected_columns = expected_energy_df.columns
        expected_index = expected_energy_df.index
        result_columns = energy_result.columns
        result_index = energy_result.index
        self.assertEqual(len(expected_columns), len(result_columns))
        self.assertEqual(len(expected_index), len(result_index))
        self.assertTrue((expected_columns == result_columns).all())
        self.assertTrue((expected_index == result_index).all())
        try:
            self.assertTrue((expected_energy_df == energy_result).all().all())
        except AssertionError:
            sys.stderr.write('\nResult:\n')
            sys.stderr.write(energy_result.to_string())
            sys.stderr.write('\nExpected:\n')
            sys.stderr.write(expected_energy_df.to_string())
            self.assertTrue((expected_energy_df == energy_result).all().all())


if __name__ == '__main__':
    unittest.main()
