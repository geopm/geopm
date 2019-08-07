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
import pandas

# TestPowerSweepAnalysis.py
def tpsa_make_mock_report_df(name_prefix, gen_val, powers):
    version = '0.3.0'
    agent = 'power_governor'
    node_name = 'mynode'
    region_id = {
        'epoch':  '9223372036854775808',
        'dgemm':  '11396693813',
        'stream': '20779751936'
    }
    start_time = 'Tue Nov  6 08:00:00 2018'

    # for input data frame
    index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
    numeric_cols = ['count', 'energy_pkg', 'energy_dram', 'frequency', 'mpi_runtime', 'runtime', 'id']

    regions = ['epoch', 'dgemm', 'stream']
    iterations = range(1, 4)

    input_data = {}
    for col in numeric_cols:
        input_data[col] = {}
        for pp in powers:
            prof_name = '{}_{}'.format(name_prefix, pp)
            for it in iterations:
                for region in regions:
                    gen_val['id'] = lambda pow: region_id[region]
                    index = (version, start_time, prof_name, agent, node_name, it, region)
                    value = gen_val[col](pp)
                    input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df


# TestNodeEfficiencyAnalysis.py
def tnea_make_mock_report_df(name_prefix, gen_val, powers, num_nodes):
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
            for power_cap in powers:
                prof_name = '{}_{}'.format(name_prefix, power_cap)
                for node in range(num_nodes):
                    node_name = 'node{}'.format(node)
                    for it in iterations:
                        for region in region_id.keys():
                            gen_val['id'] = lambda node, power_cap: region_id[region]
                            index = (version, start_time, prof_name, agent, node_name, it, region)
                            value = gen_val[col](node, power_cap)
                            input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df

# TestNodePowerAnalysis.py
def tnpa_make_mock_report_df(name_prefix, gen_val, num_nodes):
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
    prof_name = '{}_nocap'.format(name_prefix)
    agent = 'monitor'
    for col in numeric_cols:
        input_data[col] = {}
        for node in range(num_nodes):
            node_name = 'node{}'.format(node)
            for it in iterations:
                for region in region_id.keys():
                    gen_val['id'] = lambda node: region_id[region]
                    index = (version, start_time, prof_name, agent, node_name, it, region)
                    value = gen_val[col](node)
                    input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df

# TestBalancerAnalysis.py
def tba_make_mock_report_df(name_prefix, gen_val, powers):
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
        for pp in powers:
            prof_name = '{}_{}'.format(name_prefix, pp)
            for it in iterations:
                for agent in ['power_governor', 'power_balancer']:
                    for region in regions:
                        gen_val['id'] = lambda pow, agent: region_id[region]
                        index = (version, start_time, prof_name, agent, node_name, it, region)
                        value = gen_val[col](pp, agent)
                        input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df

# TestFreqSweepAnalysis.py
# TODO: profile name should affect performance. it can hide bugs if all the numbers are the same
# however the functions that generate expected output need to also take this into account
def make_mock_sweep_report_df(name_prefix, freqs, best_fit_freq, best_fit_perf,
                              metric_of_interest=None, best_fit_metric_perf=None,
                              baseline_freq=None, baseline_metric_perf=None):
    ''' Make a mock report dataframe for the fixed frequency sweeps.'''
    version = '0.3.0'
    agent = 'energy_efficient'
    node_name = 'mynode'
    region_id = {
        'epoch':  '9223372036854775808',
        'dgemm':  '11396693813',
        'stream': '20779751936'
    }
    index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
    numeric_cols = ['count', 'energy_pkg', 'frequency', 'mpi_runtime', 'runtime', 'id']
    regions = ['epoch', 'dgemm', 'stream']
    iterations = range(1, 4)
    start_time = 'Tue Nov  6 08:00:00 2018'
    input_data = {}
    # for input data frame
    gen_val = {
        'count': 1,
        'energy_pkg': 14000.0,
        'frequency': 1e9,
        'mpi_runtime': 10,
        'runtime': 50,
        'id': 'bad'
    }
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

# TestFreqSweepAnalysis.py
def tfsa_make_mock_report_df(name_prefix, metric, metric_perf):
    ''' Make a mock report dataframe for a single run.'''
    version = '0.3.0'
    agent = 'energy_efficient'
    node_name = 'mynode'
    region_id = {
        'epoch':  '9223372036854775808',
        'dgemm':  '11396693813',
        'stream': '20779751936'
    }
    index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
    numeric_cols = ['count', 'energy_pkg', 'frequency', 'mpi_runtime', 'runtime', 'id']
    regions = ['epoch', 'dgemm', 'stream']
    iterations = range(1, 4)
    start_time = 'Tue Nov  6 08:00:00 2018'

    input_data = {}
    # for input data frame
    gen_val = {
        'count': 1,
        'energy_pkg': 14000.0,
        'frequency': 1e9,
        'mpi_runtime': 10,
        'runtime': 50,
        'id': 'bad'
    }
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

