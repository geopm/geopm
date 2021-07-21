#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

"""EE_SHORT_REGION_SLOP

Integration test that executes a scaling region and a timed scaling
region back to back in a loop.  This pattern of execution is repeated
with a variety of region durations.  The goal of the test is to find
the shortest region for which the frequency map agent can successfully
change the frequency down for the timed region to save energy while
not impacting the performance of the scaling region which is targeted
for a high frequency.

"""

import sys
import unittest
import os
import glob
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import pandas

# Put integration test directory into the path
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
try:
    # Try to load geopmpy without modifiying the path
    import geopmpy.io
    import geopmpy.agent
    import geopmpy.error
    import geopmpy.hash
except ImportError:
    # If geopmpy is not installed in the PYTHONPATH then add local
    # copy to path
    from integration.test import geopm_context
    import geopmpy.io
    import geopmpy.agent
    import geopmpy.error
    import geopmpy.hash

from integration.test import util
if util.do_launch():
    # If we are not skipping the launch we need to import the test
    # launcher
    from integration.test import geopm_test_launcher

# Globals controlling uniform y axis limits in all plots
g_plot_energy_lim = [4000, 11000]
g_plot_freq_lim = [9.0e8, 2.5e9]
g_plot_time_lim = [30.0, 80.0]
g_plot_ipc_lim = [0.0, 2.4]


class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the ee_short_region_slop benchmark.

    """
    def write(self):
        """Called by the test launcher prior to executing the test application
        to write any files required by the application.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark filled in by template automatically.

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_ee_short_region_slop')

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


class TestIntegration_ee_short_region_slop(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'ee_short_region_slop'
        cls._report_path_fixed = 'test_{}_fixed.report'.format(cls._test_name)
        cls._report_path_dynamic = 'test_{}_dynamic.report'.format(cls._test_name)
        cls._trace_path_fixed = 'test_{}_fixed.trace'.format(cls._test_name)
        cls._trace_path_dynamic = 'test_{}_dynamic.trace'.format(cls._test_name)
        cls._trace_profile_path_fixed = 'test_{}_fixed.trace_profile'.format(cls._test_name)
        cls._trace_profile_path_dynamic = 'test_{}_dynamic.trace_profile'.format(cls._test_name)
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_fixed_path = 'test_{}_fixed-agent-config.json'.format(cls._test_name)
        cls._agent_conf_dynamic_path = 'test_{}_dynamic-agent-config.json'.format(cls._test_name)
        cls._num_trial = 10
        cls._num_duration = 7
        cls._num_node = 1
        cls._num_rank = 2
        cls._job_time_limit = 6000
        cls._report_signals = 'INSTRUCTIONS_RETIRED,CYCLES_REFERENCE,CYCLES_THREAD'
        cls._trace_signals = 'INSTRUCTIONS_RETIRED,MSR::UNCORE_PERF_STATUS:FREQ,TEMPERATURE_CORE'
        if not cls._skip_launch:
            cls.launch()


    @classmethod
    def launch(cls):
        """Launch the test binary for all required trials

        """
        agent_conf_fixed, agent_conf_dynamic = cls.create_agent_conf()
        trace_path_fixed = cls._trace_path_fixed
        trace_path_dynamic = cls._trace_path_dynamic
        profile_name_fixed = '{}-fixed'.format(cls._test_name)
        profile_name_dynamic = '{}-dynamic'.format(cls._test_name)
        for trial_idx in range(cls._num_trial):
            sys.stdout.write('\nTrial {} / {}\n'.format(trial_idx, cls._num_trial))
            report_path = '{}.{}'.format(cls._report_path_fixed, trial_idx)
            cls.launch_trial(agent_conf_fixed, profile_name_fixed, report_path, trace_path_fixed)
            report_path = '{}.{}'.format(cls._report_path_dynamic, trial_idx)
            cls.launch_trial(agent_conf_dynamic, profile_name_dynamic, report_path, trace_path_dynamic)
            trace_path_fixed = None
            trace_path_dynamic = None


    @classmethod
    def launch_trial(cls, agent_conf, profile_name, report_path, trace_path):
        """Launch the test binary for a single trial

        """
        app_conf = AppConf()
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    report_path,
                                                    trace_path=trace_path,
                                                    time_limit=cls._job_time_limit,
                                                    report_signals=cls._report_signals,
                                                    trace_signals=cls._trace_signals)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(cls._num_rank)
        launcher.run(profile_name)


    @classmethod
    def create_agent_conf(cls):
        """Create agent configuration objects for the fixed and dynamic cases

        """
        scaling_hash = [geopmpy.hash.crc32_str('scaling_{}'.format(idx))
                       for idx in range(cls._num_duration)]
        timed_hash = [geopmpy.hash.crc32_str('timed_{}'.format(idx))
                     for idx in range(cls._num_duration)]
        # Configure the agent
        # Query for the min and sticker frequency and run the
        # frequency map agent over this range.
        freq_min_sys = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
        freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
        freq_min = float(os.getenv('GEOPM_SLOP_FREQ_MIN', str(freq_min_sys)))
        freq_max = float(os.getenv('GEOPM_SLOP_FREQ_MAX', str(freq_sticker)))
        agent_conf_fixed_dict = {'FREQ_DEFAULT':freq_max}
        agent_conf_fixed = geopmpy.agent.AgentConf(cls._agent_conf_fixed_path,
                                                   'frequency_map',
                                                   agent_conf_fixed_dict)
        agent_conf_dynamic_dict = dict(agent_conf_fixed_dict)
        policy_idx = 0
        for hh in scaling_hash:
            agent_conf_dynamic_dict['HASH_{}'.format(policy_idx)] = hh
            agent_conf_dynamic_dict['FREQ_{}'.format(policy_idx)] = freq_max
            policy_idx += 1
        for hh in timed_hash:
            agent_conf_dynamic_dict['HASH_{}'.format(policy_idx)] = hh
            agent_conf_dynamic_dict['FREQ_{}'.format(policy_idx)] = freq_min
            policy_idx += 1

        agent_conf_dynamic = geopmpy.agent.AgentConf(cls._agent_conf_dynamic_path,
                                                     'frequency_map',
                                                     agent_conf_dynamic_dict)

        return agent_conf_fixed, agent_conf_dynamic


    def test_generate_report_plot(self):
        """Test to visualize the data in the report files

        """
        report_df = create_report_df(self._report_path_fixed,
                                     self._report_path_dynamic,
                                     self._num_trial,
                                     self._num_duration)
        out_path = 'test_{}.png'.format(self._test_name)
        generate_report_plot(report_df, out_path)


    def test_generate_trace_plot(self):
        """Test to visualize the time series data in the trace files

        """
        for region_idx in range(self._num_duration):
            scaling_region_name = 'scaling_{}'.format(region_idx)
            begin_region_hash = geopmpy.hash.crc32_str(scaling_region_name)
            timed_region_name = 'timed_{}'.format(region_idx)
            end_region_hash = geopmpy.hash.crc32_str(timed_region_name)
            region_count = range(10)
            for policy in ('dynamic', 'fixed'):
                in_path = 'test_{}_{}.trace-*'.format(self._test_name, policy)
                in_path = glob.glob(in_path)
                if len(in_path) > 1:
                    raise RuntimeError('Multiple trace files found: {}'.format(', '.join(in_path)))
                if len(in_path) == 0:
                    raise RuntimeError('No trace files found')
                in_path = in_path[0]
                trace = read_trace(in_path)
                # Get the first and last time that the regions of interest were executed
                begin_time = trace['TIME'].loc[(trace['REGION_HASH'] == begin_region_hash) &
                                               (trace['REGION_COUNT'] == region_count[0])].iloc[0]
                end_time = trace['TIME'].loc[(trace['REGION_HASH'] == end_region_hash) &
                                             (trace['REGION_COUNT'] == region_count[-1])].iloc[-1]
                delta_time = end_time - begin_time
                out_path = 'test_{}_trace_overlay_{}_{}.png'.format(self._test_name, policy, scaling_region_name)
                generate_trace_overlay_plot(trace, out_path, scaling_region_name, region_count)
                out_path = 'test_{}_trace_overlay_{}_{}.png'.format(self._test_name, policy, timed_region_name)
                generate_trace_overlay_plot(trace, out_path, timed_region_name, region_count)
                out_path = 'test_{}_trace_{}_{}.png'.format(self._test_name, policy, region_idx)
                generate_trace_plot(trace, out_path, delta_time, begin_time)


def create_report_df(report_path_fixed, report_path_dynamic, num_trial, num_duration):
    """Create pandas data frame that holds data from the reports that
       relevent to the test

    """
    result = pandas.DataFrame()
    for trial_idx in range(num_trial):
        try:
            report_path = '{}.{}'.format(report_path_fixed, trial_idx)
            report_fixed = geopmpy.io.RawReport(report_path)
            report_path = '{}.{}'.format(report_path_dynamic, trial_idx)
            report_dynamic = geopmpy.io.RawReport(report_path)
            result = result.append(create_report_trial_df(report_fixed, trial_idx, num_duration))
            result = result.append(create_report_trial_df(report_dynamic, trial_idx, num_duration))
        except:
            if trial_idx == 0:
                raise RuntimeError('Error: No reports found')
            sys.stderr.write('Warning: only {} out of {} report files have been loaded.\n'.format(trial_idx, num_trial))
            break
    # Set index for data selection
    index = ['profile-name', 'region-name', 'count']
    return result.set_index(index)


def create_report_trial_df(raw_report, trial_idx, num_duration):
    """Create a pandas data frame that holds the data from two reports
       created by running one trial and adds some derived data as
       columns.

    """
    cols = ['count',
            'package-energy (joules)',
            'requested-online-frequency',
            'power (watts)',
            'runtime (sec)',
            'frequency (Hz)']
    # Extract data frame for regions
    scaling_data = create_report_region_df(raw_report, cols, 'scaling', num_duration)
    timed_data = create_report_region_df(raw_report, cols, 'timed', num_duration)
    dur = scaling_data['runtime (sec)'] / scaling_data['count']
    scaling_data['duration (sec)'] = dur
    dur = timed_data['runtime (sec)'] / timed_data['count']
    timed_data['duration (sec)'] = dur
    prof_name = raw_report.meta_data()['Profile']
    scaling_data['profile-name'] = prof_name
    timed_data['profile-name'] = prof_name
    scaling_data['trial-idx'] = trial_idx
    timed_data['trial-idx'] = trial_idx
    return scaling_data.append(timed_data)


def create_report_region_df(raw_report, cols, key, num_duration):
    """Create a pandas data frame that holds the data from a single
       report.

    """
    host = raw_report.host_names()[0]
    region_names = raw_report.region_names(host)
    regions = [(rn, raw_report.raw_region(host, rn))
               for rn in region_names
               if rn.split('_')[0] == key and
                  int(rn.split('_')[1]) < num_duration]
    result = {}
    sample = []
    for (rn, rr) in regions:
        # Split off the prefix from the region name, leaving just
        # scaling or timed.
        sample.append(rn.split('_')[0])
    result['region-name'] = sample
    for cc in cols:
        sample = []
        for (rn, rr) in regions:
             try:
                 sample.append(raw_report.get_field(rr, cc))
             except KeyError:
                 sample.append(None)
        result[cc] = sample
    return pandas.DataFrame(result)


def generate_report_plot(report_df, out_path):
    """Generate the figure containing all of the report based plots.

    """
    plt.figure(figsize=(11,16))
    region_names = ['scaling', 'timed']
    yaxis_names = ['package-energy (joules)',
                   'frequency (Hz)',
                   'runtime (sec)']
    ylim_list = [g_plot_energy_lim,
                 g_plot_freq_lim,
                 g_plot_time_lim]
    plot_idx = 1
    for ya, ylim in zip(yaxis_names, ylim_list):
        for rn in region_names:
            ax = plt.subplot(3, 2, plot_idx)
            ax.set_xscale('log')
            generate_report_subplot(report_df, rn, 'duration (sec)', ya)
            plot_idx += 1
        plt.ylim(ylim)
        plt.subplot(3, 2, plot_idx - 2)
        plt.ylim(ylim)
    plt.savefig(out_path)
    plt.close()


def generate_report_subplot(report_df, region_type, xaxis, yaxis, ylim=None):
    """Create one of the subplots in the report figure.

    """
    for policy_type in ('fixed', 'dynamic'):
        prof_name = 'ee_short_region_slop-{}'.format(policy_type)
        level = ('profile-name', 'region-name')
        key = (prof_name, region_type)
        selected_data = report_df.xs(key=key, level=level).groupby('count')
        xdata = selected_data.mean()[xaxis]
        ydata = selected_data.mean()[yaxis]
        ydata_std = selected_data.std()[yaxis]
        plt.errorbar(xdata, ydata, ydata_std, fmt='.')
        if ylim is not None:
            plt.ylim(ylim)
    plt.title('{} region {}'.format(region_type, yaxis))
    plt.xlabel(xaxis)
    plt.ylabel(yaxis)
    plt.legend(('fixed', 'dynamic'))


def read_trace(path):
    """Parse trace file into a data frame and convert the hash values to
       integers.

    """
    converters={'REGION_HASH': lambda xx: int(xx, 16)}
    skiprows = 0
    with open(path) as fid:
        for ll in fid:
            if ll.startswith('#'):
                skiprows += 1
            else:
                break
    return pandas.read_csv(path, sep='|', skiprows=skiprows, converters=converters)


def get_ipc(trace):
    di = trace['INSTRUCTIONS_RETIRED'].diff()
    dc = trace['CYCLES_THREAD'].diff()
    return di / dc


def get_freq(trace):
    return trace['FREQUENCY']


def get_temperature(trace):
    return trace['TEMPERATURE_CORE']


def get_relative_time(trace):
    return trace['TIME'] - trace['TIME'].iloc[0]


def get_uncore_freq(trace):
    return trace['MSR::UNCORE_PERF_STATUS:FREQ']


def select_region_count(trace, region_count):
    return trace.loc[trace['REGION_COUNT'] == region_count]


def select_region(trace, region_prefix, extension=''):
    """e.g.:
    >>> select_region(trace, 'scaling_2')
    >>> select_region(trace, 'scaling_', range(12))
    >>> select_region(trace, '', ['scaling_3', 'timed_3'])

    """
    if type(extension) is str:
        extension = [extension]
    region_hash = []
    for ext in extension:
        region_name = '{}{}'.format(region_prefix, ext)
        region_hash.append(geopmpy.hash.crc32_str(region_name))
    return trace.loc[trace['REGION_HASH'].isin(region_hash)]


def select_time_range(trace, delta_time, begin_time=None):
    if begin_time is None:
        begin_time = trace['TIME'].iloc[0]
    end_time = begin_time + delta_time
    return trace.loc[(trace['TIME'] >= begin_time) &
                     (trace['TIME'] < end_time)]


def generate_trace_overlay_plot(trace, out_path, region_name, region_count):
    if type(region_count) is int:
        region_count = [region_count]
    ipc_legend = []
    freq_legend = []
    plt.figure(figsize=(11,11))
    plt.title(region_name)
    for rc in region_count:
        selected_trace = select_region(trace, region_name)
        selected_trace = select_region_count(selected_trace, rc)
        ipc = get_ipc(selected_trace)
        freq = get_freq(selected_trace)
        time = get_relative_time(selected_trace)
        plt.subplot(2, 1, 1)
        plt.plot(time.iloc[:-1], ipc.iloc[1:], '.-')
        plt.ylim(g_plot_ipc_lim)
        plt.xlabel('time since region start (sec)')
        plt.ylabel('IPC')
        ipc_legend.append('count {}'.format(rc))
        plt.subplot(2, 1, 2)
        plt.plot(time.iloc[:-1], freq.iloc[1:], '.-')
        plt.xlabel('time since region start (sec)')
        plt.ylabel('CPU freq (Hz)')
        plt.ylim(g_plot_freq_lim)
        freq_legend.append('count {}'.format(rc))
    plt.subplot(2,1,1)
    plt.legend(ipc_legend)
    plt.subplot(2,1,2)
    plt.legend(freq_legend)
    plt.savefig(out_path)
    plt.close()


def generate_trace_plot(trace, out_path, delta_time, begin_time=0):
    plt.figure(figsize=(11,16))
    trace = select_time_range(trace, delta_time, begin_time)
    yfunc_list = [get_ipc, get_freq, get_uncore_freq, get_temperature]
    ylabel_list = ['IPC', 'CPU freq (Hz)', 'Uncore freq (Hz)', 'Temperature (C)']
    ylim_list = [g_plot_ipc_lim, g_plot_freq_lim, g_plot_freq_lim, [0,120]]
    plot_idx_list = range(1, 5)
    for yfunc, ylabel, plot_idx, ylim in zip(yfunc_list, ylabel_list, plot_idx_list, ylim_list):
        plt.subplot(4, 1, plot_idx)
        for prefix in ['timed_', 'scaling_']:
            selected_trace = select_region(trace, prefix, range(12))
            ydata = yfunc(selected_trace)
            time = selected_trace['TIME']
            plt.plot(time, ydata, '.')
            plt.ylim(ylim)
            plt.xlabel('time (sec)')
            plt.ylabel(ylabel)
    plt.savefig(out_path)
    plt.close()


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
