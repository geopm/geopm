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
from analysis_helper import *


region_id = {
    'epoch':  '9223372036854775808',
    'dgemm':  '11396693813',
    'stream': '20779751936'
}
version = '0.3.0'
start_time = 'Tue Nov  6 08:00:00 2018'
power_budget = 400
agent = 'energy_efficient'
node_name = 'mynode'

# for input data frame
index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
numeric_cols = ['count', 'energy_pkg', 'frequency', 'mpi_runtime', 'runtime', 'id']
gen_val = {
    'count': 1,
    'energy_pkg': 14000.0,
    'frequency': 1e9,
    'mpi_runtime': 10,
    'runtime': 50,
    'id': 'bad'
}
ratio_inds = [0, 1, 2, 3, 6, 5, 4]
regions = ['epoch', 'dgemm', 'stream']
iterations = range(1, 4)


# TODO: profile name should affect performance. it can hide bugs if all the numbers are the same
# however the functions that generate expected output need to also take this into account
def make_mock_sweep_report_df(name_prefix, freqs, best_fit_freq, best_fit_perf,
                              metric_of_interest=None, best_fit_metric_perf=None,
                              baseline_freq=None, baseline_metric_perf=None):
    ''' Make a mock report dataframe for the fixed frequency sweeps.'''
    input_data = {}
    for col in numeric_cols:
        input_data[col] = {}
        for freq in freqs:
            prof_name = '{}_freq_{}'.format(name_prefix, freq)
            for it in iterations:
                for region in regions:
                    gen_val['id'] = region_id[region]  # return unique region id
                    index = (version, start_time, prof_name, agent, node_name, it, region)
                    value = gen_val[col]
                    # force best performance for requested best fit freq
                    if col == 'runtime':
                        if freq == best_fit_freq[region]:
                            value = best_fit_perf[region]
                        else:
                            # make other frequencies have worse performance
                            value = best_fit_perf[region] * 2.0
                    elif metric_of_interest == col:
                        if freq == best_fit_freq[region]:
                            value = best_fit_metric_perf[region]
                        elif baseline_freq and baseline_metric_perf and freq == baseline_freq:
                            value = baseline_metric_perf[region]
                    input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df


def make_mock_report_df(name_prefix, metric, metric_perf):
    ''' Make a mock report dataframe for a single run.'''
    input_data = {}
    for col in numeric_cols:
        input_data[col] = {}
        for it in iterations:
            for region in regions:
                gen_val['id'] = region_id[region]  # return unique region id
                index = (version, start_time, name_prefix, agent, node_name, it, region)
                value = gen_val[col]
                if col == metric:
                    value = metric_perf[region]
                input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df


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
        self._offline_analysis = geopmpy.analysis.OfflineBaselineComparisonAnalysis(**config)
        self._online_analysis = geopmpy.analysis.OnlineBaselineComparisonAnalysis(**config)
        self._mix_analysis = geopmpy.analysis.StreamDgemmMixAnalysis(**config)
        self._tmp_files = []

    def tearDown(self):
        for ff in self._tmp_files:
            try:
                os.remove(ff)
            except OSError:
                pass

    def test_region_freq_map(self):
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}
        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        parse_out = make_mock_sweep_report_df(self._name_prefix, self._freqs,
                                              best_fit_freq, best_fit_perf)

        parse_out = MockAppOutput(parse_out)
        result = self._sweep_analysis._region_freq_map(parse_out)

        for region in ['epoch', 'dgemm', 'stream']:
            self.assertEqual(best_fit_freq[region], result[region])

    def test_offline_baseline_comparison_report(self):
        baseline_freq = max(self._freqs)
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}

        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        # injected energy values
        baseline_metric_perf = {'dgemm': 23456.0, 'stream': 34567.0, 'epoch': 45678.0}
        best_fit_metric_perf = {'dgemm': 23000.0, 'stream': 34000.0, 'epoch': 45000.0}
        optimal_metric_perf = {'dgemm': 22345.0, 'stream': 33456.0, 'epoch': 44567.0}

        sweep_reports = make_mock_sweep_report_df(self._name_prefix, self._freqs,
                                                  best_fit_freq, best_fit_perf,
                                                  'energy_pkg', best_fit_metric_perf,
                                                  baseline_freq, baseline_metric_perf)

        prof_name = self._name_prefix + '_offline'
        single_run_report = make_mock_report_df(prof_name, 'energy_pkg', optimal_metric_perf)
        parse_out = sweep_reports.append(single_run_report)
        parse_out.sort_index(ascending=True, inplace=True)
        parse_out = MockAppOutput(sweep_reports), MockAppOutput(parse_out)

        _, _, energy_result = self._offline_analysis.summary_process(parse_out)
        expected_energy_df = get_expected_baseline_output_df([prof_name], 'energy_pkg',
                                                             baseline_metric_perf,
                                                             {prof_name: optimal_metric_perf})

        result = energy_result.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
        expected = float(expected_energy_df.loc[prof_name])
        self.assertEqual(expected, result)

    def test_online_baseline_comparison_report(self):
        baseline_freq = max(self._freqs)
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}

        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        # injected energy values
        baseline_metric_perf = {'dgemm': 23456.0, 'stream': 34567.0, 'epoch': 45678.0}
        best_fit_metric_perf = {'dgemm': 23000.0, 'stream': 34000.0, 'epoch': 45000.0}
        optimal_metric_perf = {'dgemm': 22345.0, 'stream': 33456.0, 'epoch': 44567.0}

        sweep_reports = make_mock_sweep_report_df(self._name_prefix, self._freqs,
                                                  best_fit_freq, best_fit_perf,
                                                  'energy_pkg', best_fit_metric_perf,
                                                  baseline_freq, baseline_metric_perf)

        prof_name = self._name_prefix + '_online'
        single_run_report = make_mock_report_df(prof_name, 'energy_pkg', optimal_metric_perf)
        parse_out = sweep_reports.append(single_run_report)
        parse_out.sort_index(ascending=True, inplace=True)
        parse_out = MockAppOutput(sweep_reports), MockAppOutput(parse_out)

        _, _, energy_result = self._online_analysis.summary_process(parse_out)

        expected_energy_df = get_expected_baseline_output_df([prof_name], 'energy_pkg',
                                                             baseline_metric_perf,
                                                             {prof_name: optimal_metric_perf})

        result = energy_result.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
        expected = float(expected_energy_df.loc[prof_name])
        self.assertEqual(expected, result)

    def test_stream_dgemm_mix_report(self):
        baseline_freq = self._max_freq
        best_fit_freq = {'dgemm': self._max_freq, 'stream': self._min_freq, 'epoch': self._mid_freq}
        best_fit_perf = {'dgemm': 23.0, 'stream': 34.0, 'epoch': 45.0}

        # injected energy values
        baseline_metric_perf = {'dgemm': 23456.0, 'stream': 34567.0, 'epoch': 45678.0}
        best_fit_metric_perf = {'dgemm': 23000.0, 'stream': 34000.0, 'epoch': 45000.0}
        offline_metric_perf = {'dgemm': 12345.0, 'stream': 23456.0, 'epoch': 34567.0}
        online_metric_perf = {'dgemm': 22345.0, 'stream': 33456.0, 'epoch': 44567.0}

        sweep_reports = {}
        offline_reports = {}
        online_reports = {}
        expected_energy_df = pandas.DataFrame()
        for mix_idx in ratio_inds:
            name = self._name_prefix + '_mix_{}'.format(mix_idx)
            self._tmp_files.append(name + '_app.config')
            sweep_df = make_mock_sweep_report_df(name, self._freqs,
                                                               best_fit_freq, best_fit_perf,
                                                               'energy_pkg', best_fit_metric_perf,
                                                               baseline_freq, baseline_metric_perf)
            sweep_reports[mix_idx] = MockAppOutput(sweep_df)
            offline_df = make_mock_report_df(name+'_offline', 'energy_pkg', offline_metric_perf)
            offline_reports[mix_idx] = MockAppOutput(offline_df)
            online_df = make_mock_report_df(name+'_online', 'energy_pkg', online_metric_perf)
            online_reports[mix_idx] = MockAppOutput(online_df)

            profiles = ['freq_'+str(best_fit_freq['epoch']), 'offline', 'online']
            perfs = [best_fit_metric_perf, offline_metric_perf, online_metric_perf]
            metric_perfs = {}

            profile_names = []
            for ii, prof in enumerate(profiles):
                profile_name = '{}_{}'.format(name, prof)
                profile_names.append(profile_name)
                metric_perfs[profile_name] = perfs[ii]

            energy_df = get_expected_mix_output_df(profile_names, 'energy_pkg',
                                                   baseline_metric_perf,
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
