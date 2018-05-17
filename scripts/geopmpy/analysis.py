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

"""
GEOPM Analysis - Used to run applications and analyze results for specific GEOPM use cases.
"""

import argparse
import sys
import os
import glob
from collections import defaultdict
import socket
import geopmpy.io
import geopmpy.launcher
import geopmpy.plotter
from geopmpy import __version__

import pandas


def sys_freq_avail():
    """
    Returns a list of the available frequencies on the current platform.
    """
    step_freq = 100e6
    with open('/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq') as fid:
        min_freq = 1e3 * float(fid.readline())
    with open('/proc/cpuinfo') as fid:
        for line in fid.readlines():
            if line.startswith('model name\t:'):
                sticker_freq = float(line.split('@')[1].split('GHz')[0]) * 1e9
                break
        max_freq = sticker_freq + step_freq

    num_step = 1 + int((max_freq - min_freq) / step_freq)
    result = [step_freq * ss + min_freq for ss in range(num_step)]
    return result


def get_freq_profiles(df, prefix):
    """Finds all profiles from fixed frequency runs and returns the
    frequencies and profiles in decending order. Profile names should be formatted
    according to fixed_freq_name().
    """
    profile_name_list = df.index.get_level_values('name').unique().tolist()
    freq_list = [float(pn.split('_freq_')[-1].split('_')[0])
                 for pn in profile_name_list
                 if prefix in pn and '_freq_' in pn.split(prefix)[1]]
    freq_pname = zip(freq_list, profile_name_list)
    freq_pname.sort(reverse=True)
    return freq_pname


def fixed_freq_name(prefix, freq):
    """Returns the formatted name for fixed frequency runs."""
    return '{}_freq_{}'.format(prefix, freq)


def profile_to_freq_mhz(df):
    profile_name_map = {}
    names_list = df.index.get_level_values('name').unique().tolist()
    for name in names_list:
        profile_name_map[name] = int(float(name.split('_freq_')[-1]) * 1e-6)
    df = df.rename(profile_name_map)
    df.index = df.index.set_names('freq_mhz', level='name')
    return df


def all_region_data_pretty(combined_df):
    rs = 'All region data:\n'
    for region, df in combined_df.groupby('region'):
        df = df.sort_index(ascending=False)
        rs += '-' * 120 + '\n'
        rs += 'Region : {}\n'.format(region)
        rs += '{}\n'.format(df)
    rs += '-' * 120 + '\n'
    return rs


class Analysis(object):
    """
    Base class for different types of analysis that use the data from geopm
    reports and/or logs. Implementations should define how to launch experiments,
    parse and process the output, and process text reports or graphical plots.
    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv, verbose=True, iterations=1):
        self._name = name
        self._output_dir = output_dir
        self._verbose = verbose
        self._iterations = iterations
        self._num_rank = num_rank
        self._num_node = num_node
        self._app_argv = app_argv
        self._report_paths = []
        self._trace_paths = []
        if not os.path.exists(self._output_dir):
            os.makedirs(self._output_dir)

    def launch(self):
        """
        Run experiment and set data paths corresponding to output.
        """
        raise NotImplementedError('Analysis base class does not implement the launch method()')

    def find_files(self, search_pattern='*.report'):
        """
        Uses the output dir and any custom naming convention to load the report and trace data
        produced by launch.
        """
        report_glob = os.path.join(self._output_dir, self._name + search_pattern)
        self.set_data_paths(glob.glob(report_glob))

    def set_data_paths(self, report_paths, trace_paths=None):
        """
        Set directory paths for report and trace files to be used in the analysis.
        """
        if not self._report_paths and not self._trace_paths:
            self._report_paths = report_paths
            self._trace_paths = trace_paths
        else:
            self._report_paths.extend(report_paths)
            if trace_paths is not None:
                self._trace_paths.extend(trace_paths)

    def parse(self):
        """
        Load any necessary data from the application result files into memory for analysis.
        """
        return geopmpy.io.AppOutput(self._report_paths, self._trace_paths, verbose=self._verbose)

    def plot_process(self, parse_output):
        """
        Process the parsed data into a form to be used for plotting (e.g. pandas dataframe).
        """
        raise NotImplementedError('Analysis base class does not implement the plot_process method()')

    def report_process(self, parse_output):
        """
        Process the parsed data into a form to be used for the text report.
        """
        raise NotImplementedError('Analysis base class does not implement the report_process method()')

    def report(self, process_output):
        """
        Print a text report of the results.
        """
        raise NotImplementedError('Analysis base class does not implement the report method()')

    def plot(self, process_output):
        """
        Generate graphical plots of the results.
        """
        raise NotImplementedError('Analysis base class does not implement the plot method()')


class BalancerAnalysis(Analysis):
    pass


class FreqSweepAnalysis(Analysis):
    """
    Runs the application at each available frequency. Compared to the baseline
    frequency, finds the lowest frequency for each region at which the performance
    will not be degraded by more than a given margin.
    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv, verbose=True, iterations=1, enable_turbo=False):
        super(FreqSweepAnalysis, self).__init__(name, output_dir, num_rank, num_node, app_argv, verbose, iterations)
        self._perf_margin = 0.1
        self._enable_turbo = enable_turbo

    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        ctl_conf = geopmpy.io.CtlConf(self._name + '_ctl.config',
                                      'dynamic',
                                      {'power_budget': 400,
                                       'tree_decider': 'static_policy',
                                       'leaf_decider': 'efficient_freq',
                                       'platform': 'rapl'})

        if os.getenv("GEOPM_AGENT", None) is None:
            ctl_conf.write()

        if 'GEOPM_EFFICIENT_FREQ_RID_MAP' in os.environ:
            del os.environ['GEOPM_EFFICIENT_FREQ_RID_MAP']
        if 'GEOPM_EFFICIENT_FREQ_ONLINE' in os.environ:
            del os.environ['GEOPM_EFFICIENT_FREQ_ONLINE']

        freqs = sys_freq_avail()
        for iteration in range(self._iterations):
            for freq in freqs:
                profile_name = fixed_freq_name(self._name, freq)
                report_path = os.path.join(self._output_dir, profile_name + '_{}.report'.format(iteration))
                trace_path = os.path.join(self._output_dir, profile_name + '_{}.trace'.format(iteration))
                self._report_paths.append(report_path)
                if os.getenv("GEOPM_AGENT", None) is not None:
                    with open(ctl_conf.get_path(), "w") as outfile:
                        outfile.write("{{\"FREQ_MIN\" : {}, \"FREQ_MAX\" : {}}}\n".format(freq, freq))
                if self._app_argv and not os.path.exists(report_path):
                    os.environ['GEOPM_EFFICIENT_FREQ_MIN'] = str(freq)
                    os.environ['GEOPM_EFFICIENT_FREQ_MAX'] = str(freq)
                    argv = ['dummy', '--geopm-ctl', geopm_ctl,
                                     '--geopm-policy', ctl_conf.get_path(),
                                     '--geopm-report', report_path,
                                     '--geopm-trace', trace_path,
                                     '--geopm-profile', profile_name]
                    if do_geopm_barrier:
                        argv.append('--geopm-barrier')
                    argv.append('--')
                    argv.extend(self._app_argv)
                    launcher = geopmpy.launcher.factory(argv, self._num_rank, self._num_node)
                    launcher.run()
                elif os.path.exists(report_path):
                    sys.stderr.write('<geopmpy>: Warning: output file "{}" exists, skipping run.\n'.format(report_path))
                else:
                    raise RuntimeError('<geopmpy>: output file "{}" does not exist, but no application was specified.\n'.format(report_path))

    def find_files(self):
        super(FreqSweepAnalysis, self).find_files('_freq_*.report')

    def report_process(self, parse_output):
        output = {}
        report_df = parse_output.get_report_df()
        output['region_freq_map'] = self._region_freq_map(report_df)
        output['means_df'] = self._region_means_df(report_df)
        return output

    def report(self, process_output):
        if self._verbose:
            sys.stdout.write(all_region_data_pretty(process_output['means_df']))

        region_freq_str = self._region_freq_str(process_output['region_freq_map'])
        sys.stdout.write('Region frequency map: \n    {}\n'.format(region_freq_str.replace(',', '\n    ')))

    def plot_process(self, parse_output):
        regions = parse_output.get_report_df().index.get_level_values('region').unique().tolist()
        return {region: self._runtime_energy_sweep(parse_output.get_report_df(), region)
                for region in regions}

    def plot(self, process_output):
        for region, df in process_output.iteritems():
            geopmpy.plotter.generate_runtime_energy_plot(df, region, self._output_dir)

    def _region_freq_map(self, report_df):
        """
        Calculates the best-fit frequencies for each region for a single
        mix ratio.
        """
        optimal_freq = dict()
        min_runtime = dict()

        freq_pname = get_freq_profiles(report_df, self._name)

        is_once = True
        for freq, profile_name in freq_pname:

            # Since we are still attempting to perform a run in the turbo range, we need to skip this run when
            # determining the best per region frequencies below.  The profile_name that corresponds to the
            # turbo run is always the first in the list.
            if not self._enable_turbo and (freq, profile_name) == freq_pname[0]:
                continue

            region_mean_runtime = report_df.loc[pandas.IndexSlice[:, profile_name, :, :, :, :, :, :], ].groupby(level='region')
            for region, region_df in region_mean_runtime:

                runtime = region_df['runtime'].mean()

                if is_once:
                    min_runtime[region] = runtime
                    optimal_freq[region] = freq
                elif min_runtime[region] > runtime:
                    min_runtime[region] = runtime
                    optimal_freq[region] = freq
                elif min_runtime[region] * (1.0 + self._perf_margin) > runtime:
                    optimal_freq[region] = freq
            is_once = False
        return optimal_freq

    def _region_freq_str(self, region_freq_map):
        """
        Format the mapping of region names to their best-fit frequencies.
        """
        result = ['{}:{}'.format(key, value)
                  for (key, value) in region_freq_map.iteritems()]
        result = ','.join(result)
        return result

    def _runtime_energy_sweep(self, df, region):
        freq_pname = get_freq_profiles(df, self._name)
        data = []
        freqs = []
        for freq, profile_name in freq_pname:
            freqs.append(freq)
            freq_df = df.loc[pandas.IndexSlice[:, profile_name, :, :, :, :, :, :], ]

            region_mean_runtime = freq_df.groupby(level='region')['runtime'].mean()
            region_mean_energy = freq_df.groupby(level='region')['energy'].mean()

            data.append([region_mean_runtime[region],
                         region_mean_energy[region]])

        return pandas.DataFrame(data,
                                index=freqs,
                                columns=['runtime', 'energy'])

    def _region_means_df(self, report_df):
        idx = pandas.IndexSlice

        report_df = profile_to_freq_mhz(report_df)

        cols = ['energy', 'runtime', 'mpi_runtime', 'frequency', 'count']

        means_df = report_df.groupby(['region', 'freq_mhz'])[cols].mean()
        # Define ref_freq to be three steps back from the end of the list.  The end of the list should always be
        # the turbo frequency.
        ref_freq = report_df.index.get_level_values('freq_mhz').unique().tolist()[-4]

        # Calculate the energy/runtime comparisons against the ref_freq
        ref_energy = means_df.loc[idx[:, ref_freq], ]['energy'].reset_index(level='freq_mhz', drop=True)
        es = pandas.Series((means_df['energy'] - ref_energy) / ref_energy, name='DCEngVar_%')
        means_df = pandas.concat([means_df, es], axis=1)

        ref_runtime = means_df.loc[idx[:, ref_freq], ]['runtime'].reset_index(level='freq_mhz', drop=True)
        rs = pandas.Series((means_df['runtime'] - ref_runtime) / ref_runtime, name='TimeVar_%')
        means_df = pandas.concat([means_df, rs], axis=1)

        bs = pandas.Series(means_df['runtime'] * (1.0 + self._perf_margin), name='runtime_bound')
        means_df = pandas.concat([means_df, bs], axis=1)

        # Calculate power and kwh
        p = pandas.Series(means_df['energy'] / means_df['runtime'], name='power')
        means_df = pandas.concat([means_df, p], axis=1)

        # Modify column order so that runtime bound occurs just after runtime
        cols = means_df.columns.tolist()
        tmp = cols.pop(7)
        cols.insert(2, tmp)

        return means_df[cols]


def baseline_comparison(parse_output, comp_name):
    """Used to compare a set of runs for a profile of interest to a baseline profile including verbose data.
    """
    comp_df = parse_output.loc[pandas.IndexSlice[:, comp_name, :, :, :, :, :, :], ]
    baseline_df = parse_output.loc[parse_output.index.get_level_values('name') != comp_name]
    baseline_df = profile_to_freq_mhz(baseline_df)

    # Reduce the data
    cols = ['energy', 'runtime', 'mpi_runtime', 'frequency', 'count']
    baseline_means_df = baseline_df.groupby(['region', 'freq_mhz'])[cols].mean()
    comp_means_df = comp_df.groupby(['region', 'name'])[cols].mean()

    # Add power column
    p = pandas.Series(baseline_means_df['energy'] / baseline_means_df['runtime'], name='power')
    baseline_means_df = pandas.concat([baseline_means_df, p], axis=1)
    p = pandas.Series(comp_means_df['energy'] / comp_means_df['runtime'], name='power')
    comp_means_df = pandas.concat([comp_means_df, p], axis=1)

    # Calculate energy savings
    es = pandas.Series((baseline_means_df['energy'] - comp_means_df['energy'].reset_index('name', drop=True))\
                       / baseline_means_df['energy'], name='energy_savings') * 100
    baseline_means_df = pandas.concat([baseline_means_df, es], axis=1)

    # Calculate runtime savings
    rs = pandas.Series((baseline_means_df['runtime'] - comp_means_df['runtime'].reset_index('name', drop=True))\
                       / baseline_means_df['runtime'], name='runtime_savings') * 100
    baseline_means_df = pandas.concat([baseline_means_df, rs], axis=1)

    return baseline_means_df


class OfflineBaselineComparisonAnalysis(Analysis):
    """Compares the energy efficiency plugin in offline mode to the
    baseline at sticker frequency.  Launches freq sweep and run to be
    compared.  Uses baseline comparison function to do analysis.

    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv, verbose=True, iterations=1, enable_turbo=False):
        super(OfflineBaselineComparisonAnalysis, self).__init__(name,
                                                                output_dir,
                                                                num_rank,
                                                                num_node,
                                                                app_argv,
                                                                verbose,
                                                                iterations)
        self._sweep_analysis = FreqSweepAnalysis(self._name, output_dir, num_rank,
                                                 num_node, app_argv, verbose, iterations, enable_turbo)
        self._enable_turbo = enable_turbo
        self._sweep_parse_output = None
        self._freq_pnames = []

    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        """
        Run the frequency sweep, then run the desired comparison configuration.
        """
        ctl_conf = geopmpy.io.CtlConf(self._name + '_ctl.config',
                                      'dynamic',
                                      {'power_budget': 400,
                                       'tree_decider': 'static_policy',
                                       'leaf_decider': 'efficient_freq',
                                       'platform': 'rapl'})

        self._min_freq = min(sys_freq_avail())
        self._max_freq = max(sys_freq_avail())
        if os.getenv("GEOPM_AGENT", None) is not None:
            with open(ctl_conf.get_path(), "w") as outfile:
                outfile.write("{{\"FREQ_MIN\" : {}, \"FREQ_MAX\" : {}}}\n".format(self._min_freq, self._max_freq))
        else:
            ctl_conf.write()

        # Run frequency sweep
        self._sweep_analysis.launch(geopm_ctl, do_geopm_barrier)
        parse_output = self._sweep_analysis.parse()
        process_output = self._sweep_analysis.report_process(parse_output)
        if self._verbose:
            self._sweep_analysis.report(process_output)
        region_freq_str = self._sweep_analysis._region_freq_str(process_output['region_freq_map'])
        sys.stdout.write('Region frequency map: \n    {}\n'.format(region_freq_str.replace(',', '\n    ')))

        # Run offline frequency decider
        for iteration in range(self._iterations):
            os.environ['GEOPM_EFFICIENT_FREQ_RID_MAP'] = region_freq_str
            if 'GEOPM_EFFICIENT_FREQ_ONLINE' in os.environ:
                del os.environ['GEOPM_EFFICIENT_FREQ_ONLINE']
            profile_name = self._name + '_offline'
            report_path = os.path.join(self._output_dir, profile_name + '_{}.report'.format(iteration))
            trace_path = os.path.join(self._output_dir, profile_name + '_{}.trace'.format(iteration))
            self._report_paths.append(report_path)

            if self._app_argv and not os.path.exists(report_path):
                os.environ['GEOPM_EFFICIENT_FREQ_MIN'] = str(self._min_freq)
                os.environ['GEOPM_EFFICIENT_FREQ_MAX'] = str(self._max_freq)
                argv = ['dummy', '--geopm-ctl', geopm_ctl,
                                 '--geopm-policy', ctl_conf.get_path(),
                                 '--geopm-report', report_path,
                                 '--geopm-trace', trace_path,
                                 '--geopm-profile', profile_name]
                if do_geopm_barrier:
                    argv.append('--geopm-barrier')
                argv.append('--')
                argv.extend(self._app_argv)
                launcher = geopmpy.launcher.factory(argv, self._num_rank, self._num_node)
                launcher.run()
            elif os.path.exists(report_path):
                sys.stderr.write('<geopmpy>: Warning: output file "{}" exists, skipping run.\n'.format(report_path))
            else:
                raise RuntimeError('<geopmpy>: output file "{}" does not exist, but no application was specified.\n'.format(report_path))

    def find_files(self):
        super(OfflineBaselineComparisonAnalysis, self).find_files('_offline*.report')
        self._sweep_analysis.find_files()

    def parse(self):
        """Combines reports from the sweep with other reports to be
        compared, which are determined by the profile name passed at
        construction time.

        """
        # each keeps track of only their own report paths, so need to combine parses
        sweep_output = self._sweep_analysis.parse()
        app_output = super(OfflineBaselineComparisonAnalysis, self).parse()
        self._sweep_parse_output = sweep_output.get_report_df()

        # Print the region frequency map
        sweep_report_process = self._sweep_analysis.report_process(sweep_output)
        region_freq_str = self._sweep_analysis._region_freq_str(sweep_report_process['region_freq_map'])
        sys.stdout.write('Region frequency map: \n    {}\n\n'.format(region_freq_str.replace(',', '\n    ')))

        self._freq_pnames = get_freq_profiles(self._sweep_parse_output, self._sweep_analysis._name)

        parse_output = self._sweep_parse_output.append(app_output.get_report_df())
        parse_output.sort_index(ascending=True, inplace=True)

        return parse_output

    def report_process(self, parse_output):
        comp_name = self._name + '_offline'
        baseline_comp_df = baseline_comparison(parse_output, comp_name)
        return baseline_comp_df

    def report(self, process_output):
        name = self._name + '_offline'
        ref_freq_idx = 0 if self._enable_turbo else 1
        ref_freq = int(self._freq_pnames[ref_freq_idx][0] * 1e-6)

        rs = 'Report for {}\n\n'.format(name)
        rs += 'Energy Decrease compared to {} MHz: {:.2f}%\n'.format(ref_freq, process_output.loc[pandas.IndexSlice['epoch', ref_freq], 'energy_savings'])
        rs += 'Runtime Decrease compared to {} MHz: {:.2f}%\n\n'.format(ref_freq, process_output.loc[pandas.IndexSlice['epoch', ref_freq], 'runtime_savings'])
        rs += 'Epoch data:\n'
        rs += str(process_output.loc[pandas.IndexSlice['epoch', :], ].sort_index(ascending=False)) + '\n'
        rs += '-' * 120 + '\n'
        sys.stdout.write(rs + '\n')

        if self._verbose:
            sweep_means_df = self._sweep_analysis._region_means_df(self._sweep_parse_output)
            rs = '=' * 120 + '\n'
            rs += 'Frequency Sweep Data\n'
            rs += '=' * 120 + '\n'
            rs += all_region_data_pretty(sweep_means_df)
            rs += '=' * 120 + '\n'
            rs += 'Offline Analysis Data\n'
            rs += '=' * 120 + '\n'
            rs += all_region_data_pretty(process_output)
            sys.stdout.write(rs + '\n')

    def plot_process(self, parse_output):
        pass

    def plot(self, process_output):
        pass


class OnlineBaselineComparisonAnalysis(Analysis):
    """Compares the energy efficiency plugin in offline mode to the
    baseline at sticker frequency.  Launches freq sweep and run to be
    compared.  Uses baseline comparison class to do analysis.

    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv, verbose=True, iterations=1, enable_turbo=False):
        super(OnlineBaselineComparisonAnalysis, self).__init__(name,
                                                               output_dir,
                                                               num_rank,
                                                               num_node,
                                                               app_argv,
                                                               verbose,
                                                               iterations)
        self._sweep_analysis = FreqSweepAnalysis(self._name, output_dir, num_rank,
                                                 num_node, app_argv, verbose, iterations, enable_turbo)
        self._enable_turbo = enable_turbo
        self._freq_pnames = []

    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        """
        Run the frequency sweep, then run the desired comparison configuration.
        """
        ctl_conf = geopmpy.io.CtlConf(self._name + '_ctl.config',
                                      'dynamic',
                                      {'power_budget': 400,
                                       'tree_decider': 'static_policy',
                                       'leaf_decider': 'efficient_freq',
                                       'platform': 'rapl'})
        if os.getenv("GEOPM_AGENT", None) is None:
            ctl_conf.write()

        # Run frequency sweep
        self._sweep_analysis.launch(geopm_ctl, do_geopm_barrier)

        # Run online frequency decider
        for iteration in range(self._iterations):
            os.environ['GEOPM_EFFICIENT_FREQ_ONLINE'] = 'yes'
            if 'GEOPM_EFFICIENT_FREQ_RID_MAP' in os.environ:
                del os.environ['GEOPM_EFFICIENT_FREQ_RID_MAP']

            profile_name = self._name + '_online'
            report_path = os.path.join(self._output_dir, profile_name + '_{}.report'.format(iteration))
            trace_path = os.path.join(self._output_dir, profile_name + '_{}.trace'.format(iteration))
            self._report_paths.append(report_path)

            freqs = sys_freq_avail() # freqs contains a list of available system frequencies in ascending order
            self._min_freq = freqs[0]
            self._max_freq = freqs[-1] if self._enable_turbo else freqs[-2]
            if os.getenv("GEOPM_AGENT", None) is not None:
                with open(ctl_conf.get_path(), "w") as outfile:
                    outfile.write("{{\"FREQ_MIN\" : {}, \"FREQ_MAX\" : {}}}\n".format(self._min_freq, self._max_freq))
            if self._app_argv and not os.path.exists(report_path):
                os.environ['GEOPM_EFFICIENT_FREQ_MIN'] = str(self._min_freq)
                os.environ['GEOPM_EFFICIENT_FREQ_MAX'] = str(self._max_freq)
                argv = ['dummy', '--geopm-ctl', geopm_ctl,
                                 '--geopm-policy', ctl_conf.get_path(),
                                 '--geopm-report', report_path,
                                 '--geopm-trace', trace_path,
                                 '--geopm-profile', profile_name]
                if do_geopm_barrier:
                    argv.append('--geopm-barrier')
                argv.append('--')
                argv.extend(self._app_argv)
                launcher = geopmpy.launcher.factory(argv, self._num_rank, self._num_node)
                launcher.run()
            elif os.path.exists(report_path):
                sys.stderr.write('<geopmpy>: Warning: output file "{}" exists, skipping run.\n'.format(report_path))
            else:
                raise RuntimeError('<geopmpy>: output file "{}" does not exist, but no application was specified.\n'.format(report_path))

    def find_files(self):
        super(OnlineBaselineComparisonAnalysis, self).find_files('_online*.report')
        self._sweep_analysis.find_files()

    def parse(self):
        """Combines reports from the sweep with other reports to be
        compared, which are determined by the profile name passed at
        construction time.

        """
        # each keeps track of only their own report paths, so need to combine parses
        sweep_output = self._sweep_analysis.parse()
        app_output = super(OnlineBaselineComparisonAnalysis, self).parse()
        parse_output = sweep_output.get_report_df()

        # Print the region frequency map
        sweep_report_process = self._sweep_analysis.report_process(sweep_output)
        region_freq_str = self._sweep_analysis._region_freq_str(sweep_report_process['region_freq_map'])
        sys.stdout.write('Region frequency map: \n    {}\n\n'.format(region_freq_str.replace(',', '\n    ')))

        self._freq_pnames = get_freq_profiles(parse_output, self._sweep_analysis._name)
        parse_output = parse_output.append(app_output.get_report_df())
        parse_output.sort_index(ascending=True, inplace=True)
        return parse_output

    def report_process(self, parse_output):
        comp_name = self._name + '_online'
        baseline_comp_df = baseline_comparison(parse_output, comp_name)
        return baseline_comp_df

    def report(self, process_output):
        name = self._name + '_online'
        ref_freq_idx = 0 if self._enable_turbo else 1
        ref_freq = int(self._freq_pnames[ref_freq_idx][0] * 1e-6)

        rs = 'Report for {}\n\n'.format(name)
        rs += 'Energy Decrease compared to {} MHz: {:.2f}%\n'.format(ref_freq, process_output.loc[pandas.IndexSlice['epoch', ref_freq], 'energy_savings'])
        rs += 'Runtime Decrease compared to {} MHz: {:.2f}%\n\n'.format(ref_freq, process_output.loc[pandas.IndexSlice['epoch', ref_freq], 'runtime_savings'])
        rs += 'Epoch data:\n'
        rs += str(process_output.loc[pandas.IndexSlice['epoch', :], ].sort_index(ascending=False)) + '\n'
        rs += '-' * 120 + '\n'
        sys.stdout.write(rs + '\n')

        if self._verbose:
            rs = '=' * 120 + '\n'
            rs += 'Online Analysis Data\n'
            rs += '=' * 120 + '\n'
            rs += all_region_data_pretty(process_output)
            sys.stdout.write(rs)

    def plot_process(self, parse_output):
        pass

    def plot(self, process_output):
        pass


class StreamDgemmMixAnalysis(Analysis):
    """Runs a full frequency sweep, then offline and online modes of the
       frequency decider plugin. The energy and runtime of the
       application best-fit, per-phase offline mode, and per-phase
       online mode are compared to the run a sticker frequency.

    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv, verbose=True, iterations=1, enable_turbo=False):
        super(StreamDgemmMixAnalysis, self).__init__(name, output_dir, num_rank, num_node, app_argv, verbose, iterations)

        self._enable_turbo = enable_turbo
        self._sweep_analysis = {}
        self._offline_analysis = {}
        self._online_analysis = {}
        self._mix_ratios = [(1.0, 0.25), (1.0, 0.5), (1.0, 0.75), (1.0, 1.0),
                            (0.75, 1.0), (0.5, 1.0), (0.25, 1.0)]
        loop_count = 10
        dgemm_bigo = 20.25
        stream_bigo = 1.449
        dgemm_bigo_jlse = 35.647
        dgemm_bigo_quartz = 29.12
        stream_bigo_jlse = 1.6225
        stream_bigo_quartz = 1.7941
        hostname = socket.gethostname()
        if hostname.endswith('.alcf.anl.gov'):
            dgemm_bigo = dgemm_bigo_jlse
            stream_bigo = stream_bigo_jlse
        else:
            dgemm_bigo = dgemm_bigo_quartz
            stream_bigo = stream_bigo_quartz

        for (ratio_idx, ratio) in enumerate(self._mix_ratios):
            name = self._name + '_mix_{}'.format(ratio_idx)
            app_conf_name = name + '_app.config'
            app_conf = geopmpy.io.BenchConf(app_conf_name)
            app_conf.set_loop_count(loop_count)
            app_conf.append_region('dgemm',  ratio[0] * dgemm_bigo)
            app_conf.append_region('stream', ratio[1] * stream_bigo)
            app_conf.append_region('all2all', 1.0)
            app_conf.write()

            source_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            app_path = os.path.join(source_dir, '.libs', 'geopmbench')
            # if not found, use geopmbench from user's PATH
            if not os.path.exists(app_path):
                app_path = "geopmbench"
            app_argv = [app_path, app_conf_name]
            # Analysis class that runs the frequency sweep (will append _freq_XXXX to name)
            self._sweep_analysis[ratio_idx] = FreqSweepAnalysis(name,
                                                                self._output_dir,
                                                                self._num_rank,
                                                                self._num_node,
                                                                app_argv,
                                                                self._verbose,
                                                                self._iterations,
                                                                self._enable_turbo)
            # Analysis class that includes a full sweep plus the plugin run with freq map
            self._offline_analysis[ratio_idx] = OfflineBaselineComparisonAnalysis(name,
                                                                                  self._output_dir,
                                                                                  self._num_rank,
                                                                                  self._num_node,
                                                                                  app_argv,
                                                                                  self._verbose,
                                                                                  self._iterations,
                                                                                  self._enable_turbo)
            # Analysis class that runs the online plugin
            self._online_analysis[ratio_idx] = OnlineBaselineComparisonAnalysis(name,
                                                                                self._output_dir,
                                                                                self._num_rank,
                                                                                self._num_node,
                                                                                app_argv,
                                                                                self._verbose,
                                                                                self._iterations,
                                                                                self._enable_turbo)


    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        for (ratio_idx, ratio) in enumerate(self._mix_ratios):
            self._sweep_analysis[ratio_idx].launch(geopm_ctl, do_geopm_barrier)
            self._offline_analysis[ratio_idx].launch(geopm_ctl, do_geopm_barrier)
            self._online_analysis[ratio_idx].launch(geopm_ctl, do_geopm_barrier)

    def parse(self):
        app_output = super(StreamDgemmMixAnalysis, self).parse()
        return app_output.get_report_df()

    def report_process(self, parse_output):
        df = parse_output
        name_prefix = self._name

        runtime_data = []
        energy_data = []
        app_freq_data = []
        online_freq_data = []
        series_names = ['offline application', 'offline per-phase', 'online per-phase']
        regions = df.index.get_level_values('region').unique().tolist()

        energy_result_df = pandas.DataFrame()
        runtime_result_df = pandas.DataFrame()
        for (ratio_idx, ratio) in enumerate(self._mix_ratios):
            name = self._name + '_mix_{}'.format(ratio_idx)

            optimal_freq = self._sweep_analysis[ratio_idx]._region_freq_map(df)

            freq_temp = [optimal_freq[region]
                         for region in sorted(regions)]
            app_freq_data.append(freq_temp)

            freq_pname = get_freq_profiles(parse_output, name)

            baseline_freq, baseline_name = freq_pname[0]
            best_fit_freq = optimal_freq['epoch']
            best_fit_name = fixed_freq_name(name, best_fit_freq)

            baseline_df = df.loc[pandas.IndexSlice[:, baseline_name, :, :, :, :, :, :], ]

            best_fit_df = df.loc[pandas.IndexSlice[:, best_fit_name, :, :, :, :, :, :], ]
            combo_df = baseline_df.append(best_fit_df)
            comp_df = baseline_comparison(combo_df, best_fit_name)
            offline_app_energy = comp_df.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
            offline_app_runtime = comp_df.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'runtime_savings']

            offline_df = df.loc[pandas.IndexSlice[:, name + '_offline', :, :, :, :, :, :], ]
            combo_df = baseline_df.append(offline_df)
            comp_df = baseline_comparison(combo_df, name + '_offline')
            offline_phase_energy = comp_df.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
            offline_phase_runtime = comp_df.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'runtime_savings']

            online_df = df.loc[pandas.IndexSlice[:, name + '_online', :, :, :, :, :, :], ]
            combo_df = baseline_df.append(online_df)
            comp_df = baseline_comparison(combo_df, name + '_online')
            online_phase_energy = comp_df.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'energy_savings']
            online_phase_runtime = comp_df.loc[pandas.IndexSlice['epoch', int(baseline_freq * 1e-6)], 'runtime_savings']

            row = pandas.DataFrame([[offline_app_energy, offline_phase_energy, online_phase_energy]],
                                   columns=series_names, index=[ratio_idx])
            energy_result_df = energy_result_df.append(row)

            row = pandas.DataFrame([[offline_app_runtime, offline_phase_runtime, online_phase_runtime]],
                                   columns=series_names, index=[ratio_idx])
            runtime_result_df = runtime_result_df.append(row)

        # produce combined output
        app_freq_df = pandas.DataFrame(app_freq_data, columns=sorted(regions))
        return energy_result_df, runtime_result_df, app_freq_df

    def plot_process(self, parse_output):
        return self.report_process(parse_output)

    def report(self, process_output):
        rs = 'Report:\n'
        energy_result_df, runtime_result_df, app_freq_df = process_output
        rs += 'Energy\n'
        rs += '{}\n'.format(energy_result_df)
        rs += 'Runtime\n'
        rs += '{}\n'.format(runtime_result_df)
        rs += 'Application best-fit frequencies\n'
        rs += '{}\n'.format(app_freq_df)
        sys.stdout.write(rs)
        return rs

    def plot(self, process_output):
        sys.stdout.write('Plot:\n')
        energy_result_df, runtime_result_df, app_freq_df = process_output
        geopmpy.plotter.generate_bar_plot_sc17(energy_result_df, 'Energy-to-Solution Decrease', self._output_dir)
        geopmpy.plotter.generate_bar_plot_sc17(runtime_result_df, 'Time-to-Solution Increase', self._output_dir)
        geopmpy.plotter.generate_app_best_freq_plot_sc17(app_freq_df, 'Offline auto per-phase best-fit frequency', self._output_dir)


def main(argv):
    help_str = """
Usage: {argv_0} [-h|--help] [--version]
       {argv_0} -t ANALYSIS_TYPE -n NUM_RANK -N NUM_NODE [-o OUTPUT_DIR] [-p PROFILE_PREFIX] [-l LEVEL] -- EXEC [EXEC_ARGS]

geopmanalysis - Used to run applications and analyze results for specific
                GEOPM use cases.

  -h, --help            show this help message and exit

  -t, --analysis-type   type of analysis to perform. Available
                        ANALYSIS_TYPE values: freq_sweep, balancer,
                        offline, online, stream_mix.
  -n, --num-rank        total number of application ranks to launch with
  -N, --num-node        number of compute nodes to launch onto
  -o, --output-dir      the output directory for reports, traces, and plots (default '.')
  -p, --profile-prefix  prefix to prepend to profile name when launching
  -l, --level           controls the level of detail provided in the analysis.
                        level 0: run application and generate reports and traces only
                        level 1: print analysis of report and trace data (default)
                        level 2: create plots from report and trace data
  -s, --skip-launch     do not launch jobs, only analyze existing data
  -v, --verbose         Print verbose debugging information.
  --geopm-ctl           launch type for the GEOPM controller.  Available
                        GEOPM_CTL values: process, pthread, or application (default 'process')
  --iterations          Number of experiments to run per analysis type
  --enable-turbo        Allows turbo to be tested when determining best per-region frequencies.
                        (default disables turbo)
  --version             show the GEOPM version number and exit

""".format(argv_0=sys.argv[0])
    version_str = """\
GEOPM version {version}
Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation. All rights reserved.
""".format(version=__version__)

    if '--help' in argv or '-h' in argv:
        sys.stdout.write(help_str)
        exit(0)
    if '--version' in argv:
        sys.stdout.write(version_str)
        exit(0)

    analysis_type_map = {'freq_sweep': FreqSweepAnalysis,
                         'balancer': BalancerAnalysis,
                         'offline': OfflineBaselineComparisonAnalysis,
                         'online': OnlineBaselineComparisonAnalysis,
                         'stream_mix': StreamDgemmMixAnalysis}
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter, add_help=False)

    parser.add_argument('-t', '--analysis-type', dest='analysis_type',
                        action='store', required=True)
    parser.add_argument('-n', '--num-rank', dest='num_rank',
                        action='store', default=None, type=int)
    parser.add_argument('-N', '--num-node', dest='num_node',
                        action='store', default=None, type=int)
    parser.add_argument('-o', '--output-dir', dest='output_dir',
                        action='store', default='.')
    parser.add_argument('-p', '--profile-prefix', dest='profile_prefix',
                        action='store', default='')
    parser.add_argument('-l', '--level',
                        action='store', default=1, type=int)
    parser.add_argument('app_argv', metavar='APP_ARGV',
                        action='store', nargs='*')
    parser.add_argument('-s', '--skip-launch', dest='skip_launch',
                        action='store_true', default=False)
    parser.add_argument('--geopm-ctl', dest='geopm_ctl',
                        action='store', default='process')
    parser.add_argument('--iterations',
                        action='store', default=1, type=int)
    parser.add_argument('--enable-turbo', dest='enable_turbo',
                        action='store_true', default=False)
    parser.add_argument('-v', '--verbose',
                        action='store_true', default=False)

    args = parser.parse_args(argv)
    if args.analysis_type not in analysis_type_map:
        raise SyntaxError('Analysis type: "{}" unrecognized.'.format(args.analysis_type))

    analysis = analysis_type_map[args.analysis_type](args.profile_prefix,
                                                     args.output_dir,
                                                     args.num_rank,
                                                     args.num_node,
                                                     args.app_argv,
                                                     args.verbose,
                                                     args.iterations,
                                                     args.enable_turbo)

    if args.skip_launch:
        analysis.find_files()
    else:
        # if launching, must run within an allocation to make sure all runs use
        # the same set of nodes
        if args.num_rank is None or args.num_node is None:
            raise RuntimeError('--num-rank and --num-node are required for launch.')
        if 'SLURM_NNODES' in os.environ:
            num_node = int(os.getenv('SLURM_NNODES'))
        elif 'COBALT_NODEFILE' in os.environ:
            with open(os.getenv('COBALT_NODEFILE')) as fid:
                num_node = len(fid.readlines())
        else:
            num_node = -1
        if num_node != args.num_node:
            raise RuntimeError('Launch must be made inside of a job allocation and application must run on all allocated nodes.')

        analysis.launch(args.geopm_ctl)

    if args.level > 0:
        parse_output = analysis.parse()
        process_output = analysis.report_process(parse_output)
        analysis.report(process_output)
        if args.level > 1:
            process_output = analysis.plot_process(parse_output)
            analysis.plot(process_output)
