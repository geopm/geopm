#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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


class Analysis(object):
    """
    Base class for different types of analysis that use the data from geopm
    reports and/or logs. Implementations should define how to launch experiments,
    parse and process the output, and process text reports or graphical plots.
    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv):
        self._name = name
        self._output_dir = output_dir
        self._num_rank = num_rank
        self._num_node = num_node
        self._app_argv = app_argv
        self._report_paths = []
        self._trace_paths = []

    def launch(self):
        """
        Run experiment and set data paths corresponding to output.
        """
        raise NotImplementedError('Analysis base class does not implement the launch method()')

    def find_files(self):
        """
        Uses the output dir and any custom naming convention to load the report and trace data
        produced by launch.
        """
        raise NotImplementedError('Analysis base class does not implement the find_files method()')

    def set_data_paths(self, report_paths, trace_paths=None):
        """
        Set directory paths for report and trace files to be used in the analysis.
        """
        if not self._report_paths and not self._trace_paths:
            self._report_paths = report_paths
            self._trace_paths = trace_paths
        else:
            raise RuntimeError('Analysis object already has report and trace paths populated.')

    def parse(self):
        """
        Load any necessary data from the application result files into memory for analysis.
        """
        return geopmpy.io.AppOutput(self._report_paths, self._trace_paths, verbose=False)

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
    def __init__(self, name, output_dir, num_rank, num_node, app_argv):
        super(FreqSweepAnalysis, self).__init__(name, output_dir, num_rank, num_node, app_argv)
        self._perf_margin = 0.1

    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        ctl_conf = geopmpy.io.CtlConf(self._name + '_ctl.config',
                                      'dynamic',
                                      {'power_budget': 400,
                                       'tree_decider': 'static_policy',
                                       'leaf_decider': 'simple_freq',
                                       'platform': 'rapl'})
        ctl_conf.write()
        if 'GEOPM_SIMPLE_FREQ_RID_MAP' in os.environ:
            del os.environ['GEOPM_SIMPLE_FREQ_RID_MAP']
        if 'GEOPM_SIMPLE_FREQ_ADAPTIVE' in os.environ:
            del os.environ['GEOPM_SIMPLE_FREQ_ADAPTIVE']
        for freq in sys_freq_avail():
            profile_name = fixed_freq_name(self._name, freq)
            report_path = os.path.join(self._output_dir, profile_name + '.report')
            self._report_paths.append(report_path)
            if self._app_argv and not os.path.exists(report_path):
                os.environ['GEOPM_SIMPLE_FREQ_MIN'] = str(freq)
                os.environ['GEOPM_SIMPLE_FREQ_MAX'] = str(freq)
                argv = ['dummy', '--geopm-ctl', geopm_ctl,
                                 '--geopm-policy', ctl_conf.get_path(),
                                 '--geopm-report', report_path,
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
        report_glob = os.path.join(self._output_dir, self._name + '_freq_*.report')
        self.set_data_paths(glob.glob(report_glob))

    def report_process(self, parse_output):
        return self._region_freq_map(parse_output.get_report_df())

    def plot_process(self, parse_output):
        regions = parse_output.get_report_df().index.get_level_values('region').unique().tolist()
        return {region: self._runtime_energy_sweep(parse_output.get_report_df(), region)
                for region in regions}

    def report(self, process_output):
        region_freq_str = self._region_freq_str(process_output)
        sys.stdout.write('Region frequency map: {}\n'.format(region_freq_str))

    def plot(self, process_output):
        for region, df in process_output.iteritems():
            geopmpy.plotter.generate_power_energy_plot(df, region)

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
        freqs = sys_freq_avail()
        data = []
        for freq in freqs:
            profile_name = fixed_freq_name(self._name, freq)

            freq_df = df.loc[pandas.IndexSlice[:, profile_name, :, :, :, :, :, :], ]

            region_mean_runtime = freq_df.groupby(level='region')['runtime'].mean()
            region_mean_energy = freq_df.groupby(level='region')['energy'].mean()

            data.append([region_mean_runtime[region],
                         region_mean_energy[region]])

        return pandas.DataFrame(data,
                                index=freqs,
                                columns=['runtime', 'energy'])


def baseline_comparison(parse_output, baseline_name, comp_name):
    """Used to compare a set of runs for a profile of interest to a baseline profile.

    """
    frame = parse_output.loc[pandas.IndexSlice[:, comp_name, :, :, :, :, :, :], ]
    baseline_frame = parse_output.loc[pandas.IndexSlice[:, baseline_name, :, :, :, :, :, :], ]

    index = ['name']
    baseline_frame.reset_index(index, drop=True, inplace=True)
    frame.reset_index(index, drop=True, inplace=True)

    runtime_savings = (baseline_frame['runtime'] - frame['runtime']) / baseline_frame['runtime']
    # show runtime as positive percent; we only care about epoch region
    runtime_savings = runtime_savings.loc[pandas.IndexSlice[:, :, :, :, :, :, 'epoch'], ].mean() * -100.0
    energy_savings = (baseline_frame['energy'] - frame['energy']) / baseline_frame['energy']
    energy_savings = energy_savings.loc[pandas.IndexSlice[:, :, :, :, :, :, 'epoch'], ].mean()

    # index contains the profile name which will have things that vary between runs
    result_df = pandas.DataFrame([[energy_savings, runtime_savings]],
                                 index=[comp_name], columns=['energy', 'runtime'])
    return result_df


class OfflineBaselineComparisonAnalysis(Analysis):
    """Compares the energy efficiency plugin in offline mode to the
    baseline at sticker frequency.  Launches freq sweep and run to be
    compared.  Uses baseline comparison function to do analysis.

    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv):
        super(OfflineBaselineComparisonAnalysis, self).__init__(name,
                                                                output_dir,
                                                                num_rank,
                                                                num_node,
                                                                app_argv)
        self._sweep_analysis = FreqSweepAnalysis(self._name, output_dir, num_rank,
                                                 num_node, app_argv)

    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        """
        Run the frequency sweep, then run the desired comparison configuration.
        """
        ctl_conf = geopmpy.io.CtlConf(self._name + '_ctl.config',
                                      'dynamic',
                                      {'power_budget': 400,
                                       'tree_decider': 'static_policy',
                                       'leaf_decider': 'simple_freq',
                                       'platform': 'rapl'})
        ctl_conf.write()

        # Run frequency sweep
        self._sweep_analysis.launch(geopm_ctl, do_geopm_barrier)
        parse_output = self._sweep_analysis.parse()
        process_output = self._sweep_analysis.report_process(parse_output)
        region_freq_str = self._sweep_analysis._region_freq_str(process_output)

        # Run offline frequency decider
        os.environ['GEOPM_SIMPLE_FREQ_RID_MAP'] = region_freq_str
        if 'GEOPM_SIMPLE_FREQ_ADAPTIVE' in os.environ:
            del os.environ['GEOPM_SIMPLE_FREQ_ADAPTIVE']
        profile_name = self._name + '_offline'
        report_path = os.path.join(self._output_dir, profile_name + '.report')
        self._report_paths.append(report_path)

        self._min_freq = min(sys_freq_avail())
        self._max_freq = max(sys_freq_avail())
        if self._app_argv and not os.path.exists(report_path):
            os.environ['GEOPM_SIMPLE_FREQ_MIN'] = str(self._min_freq)
            os.environ['GEOPM_SIMPLE_FREQ_MAX'] = str(self._max_freq)
            argv = ['dummy', '--geopm-ctl', geopm_ctl,
                             '--geopm-policy', ctl_conf.get_path(),
                             '--geopm-report', report_path,
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
        report_glob = os.path.join(self._output_dir, self._name + '*.report')
        self.set_data_paths(glob.glob(report_glob))

    def parse(self):
        """Combines reports from the sweep with other reports to be
        compared, which are determined by the profile name passed at
        construction time.

        """
        # each keeps track of only their own report paths, so need to combine parses
        sweep_output = self._sweep_analysis.parse()
        app_output = super(OfflineBaselineComparisonAnalysis, self).parse()
        parse_output = sweep_output.get_report_df()
        parse_output = parse_output.append(app_output.get_report_df())
        parse_output.sort_index(ascending=True, inplace=True)
        return parse_output

    def report_process(self, parse_output):
        freq_pname = get_freq_profiles(parse_output, self._name)

        baseline_freq, baseline_name = freq_pname[0]
        comp_name = self._name + '_offline'
        baseline_comp = baseline_comparison(parse_output, baseline_name, comp_name)
        for freq, freq_name in freq_pname:
            comp = baseline_comparison(parse_output, freq_name, comp_name)
            comp.index = [freq_name]
            baseline_comp = baseline_comp.append(comp)
        baseline_comp.sort_index(ascending=True, inplace=True)
        return baseline_comp

    def report(self, process_output):
        name = self._name + '_offline'
        rs = 'Report for {}\n'.format(name)
        rs += 'Energy Decrease: {0:.2f}%\n'.format(process_output.loc[name]['energy'])
        rs += 'Runtime Increase: {0:.2f}%\n'.format(process_output.loc[name]['runtime'])
        rs += 'All frequencies:\n'
        rs += str(process_output)+'\n'
        sys.stdout.write(rs)


# TODO: this only needs to run the sticker freq, not the whole sweep
class OnlineBaselineComparisonAnalysis(Analysis):
    """Compares the energy efficiency plugin in offline mode to the
    baseline at sticker frequency.  Launches freq sweep and run to be
    compared.  Uses baseline comparison class to do analysis.

    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv):
        super(OnlineBaselineComparisonAnalysis, self).__init__(name,
                                                               output_dir,
                                                               num_rank,
                                                               num_node,
                                                               app_argv)
        self._sweep_analysis = FreqSweepAnalysis(self._name, output_dir, num_rank,
                                                 num_node, app_argv)

    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        """
        Run the frequency sweep, then run the desired comparison configuration.
        """
        ctl_conf = geopmpy.io.CtlConf(self._name + '_ctl.config',
                                      'dynamic',
                                      {'power_budget': 400,
                                       'tree_decider': 'static_policy',
                                       'leaf_decider': 'simple_freq',
                                       'platform': 'rapl'})
        ctl_conf.write()

        # Run frequency sweep
        self._sweep_analysis.launch(geopm_ctl, do_geopm_barrier)
        parse_output = self._sweep_analysis.parse()
        process_output = self._sweep_analysis.report_process(parse_output)
        region_freq_str = self._sweep_analysis._region_freq_str(process_output)

        # Run online frequency decider
        os.environ['GEOPM_SIMPLE_FREQ_RID_ADAPTIVE'] = 'yes'
        if 'GEOPM_SIMPLE_FREQ_MAP' in os.environ:
            del os.environ['GEOPM_SIMPLE_FREQ_MAP']

        profile_name = self._name + '_online'
        report_path = os.path.join(self._output_dir, profile_name + '.report')
        self._report_paths.append(report_path)

        self._min_freq = min(sys_freq_avail())
        self._max_freq = max(sys_freq_avail())
        if self._app_argv and not os.path.exists(report_path):
            os.environ['GEOPM_SIMPLE_FREQ_MIN'] = str(self._min_freq)
            os.environ['GEOPM_SIMPLE_FREQ_MAX'] = str(self._max_freq)
            argv = ['dummy', '--geopm-ctl', geopm_ctl,
                             '--geopm-policy', ctl_conf.get_path(),
                             '--geopm-report', report_path,
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
        report_glob = os.path.join(self._output_dir, self._name + '*.report')
        self.set_data_paths(glob.glob(report_glob))

    def parse(self):
        """Combines reports from the sweep with other reports to be
        compared, which are determined by the profile name passed at
        construction time.

        """
        # each keeps track of only their own report paths, so need to combine parses
        sweep_output = self._sweep_analysis.parse()
        app_output = super(OnlineBaselineComparisonAnalysis, self).parse()
        parse_output = sweep_output.get_report_df()
        parse_output = parse_output.append(app_output.get_report_df())
        parse_output.sort_index(ascending=True, inplace=True)
        return parse_output

    def report_process(self, parse_output):
        freq_pname = get_freq_profiles(parse_output, self._name)

        baseline_freq, baseline_name = freq_pname[0]
        comp_name = self._name + '_online'
        baseline_comp = baseline_comparison(parse_output, baseline_name, comp_name)
        for freq, freq_name in freq_pname:
            comp = baseline_comparison(parse_output, freq_name, comp_name)
            comp.index = [freq_name]
            baseline_comp = baseline_comp.append(comp)
        baseline_comp.sort_index(ascending=True, inplace=True)
        return baseline_comp

    def report(self, process_output):
        name = self._name + '_online'
        rs = 'Report for {}\n'.format(name)
        rs += 'Energy Decrease: {0:.2f}%\n'.format(process_output.loc[name]['energy'])
        rs += 'Runtime Increase: {0:.2f}%\n'.format(process_output.loc[name]['runtime'])
        rs += 'All frequencies:\n'
        rs += str(process_output)+'\n'
        sys.stdout.write(rs)


class StreamDgemmMixAnalysis(Analysis):
    """Runs a full frequency sweep, then offline and online modes of the
       frequency decider plugin. The energy and runtime of the
       application best-fit, per-phase offline mode, and per-phase
       online mode are compared to the run a sticker frequency.

    """
    def __init__(self, name, output_dir, num_rank, num_node, app_argv):
        super(StreamDgemmMixAnalysis, self).__init__(name, output_dir, num_rank, num_node, app_argv)

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
                                                                app_argv)
            # Analysis class that includes a full sweep plus the plugin run with freq map
            self._offline_analysis[ratio_idx] = OfflineBaselineComparisonAnalysis(name,
                                                                                  self._output_dir,
                                                                                  self._num_rank,
                                                                                  self._num_node,
                                                                                  app_argv)
            # Analysis class that runs the online plugin
            self._online_analysis[ratio_idx] = OnlineBaselineComparisonAnalysis(name,
                                                                                self._output_dir,
                                                                                self._num_rank,
                                                                                self._num_node,
                                                                                app_argv)


    def launch(self, geopm_ctl='process', do_geopm_barrier=False):
        for (ratio_idx, ratio) in enumerate(self._mix_ratios):
            self._sweep_analysis[ratio_idx].launch(geopm_ctl, do_geopm_barrier)
            self._offline_analysis[ratio_idx].launch(geopm_ctl, do_geopm_barrier)
            self._online_analysis[ratio_idx].launch(geopm_ctl, do_geopm_barrier)

    def find_files(self):
        report_glob = os.path.join(self._output_dir, self._name + '*.report')
        self.set_data_paths(glob.glob(report_glob))

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
            offline_app_df = baseline_comparison(df, baseline_name, best_fit_name)
            offline_phase_df = self._offline_analysis[ratio_idx].report_process(df)
            online_phase_df = self._online_analysis[ratio_idx].report_process(df)
            offline_app_energy = float(offline_app_df['energy'])
            offline_phase_energy = float(offline_phase_df.loc[name+'_offline']['energy'])
            online_phase_energy = float(online_phase_df.loc[name+'_online']['energy'])
            row = pandas.DataFrame([[offline_app_energy, offline_phase_energy, online_phase_energy]],
                                   columns=series_names, index=[ratio_idx])
            energy_result_df = energy_result_df.append(row)

            offline_app_runtime = float(offline_app_df['runtime'])
            offline_phase_runtime = float(offline_phase_df.loc[name+'_offline']['runtime'])
            online_phase_runtime = float(online_phase_df.loc[name+'_online']['runtime'])
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

  -t, --analysis_type   type of analysis to perform. Available
                        ANALYSIS_TYPE values: freq_sweep, balancer,
                        offline, online, stream_mix.
  -n, --num_rank        total number of application ranks to launch with
  -N, --num_node        number of compute nodes to launch onto
  -o, --output_dir      the output directory for reports, traces, and plots (default '.')
  -p, --profile_prefix  prefix to prepend to profile name when launching
  -l, --level           controls the level of detail provided in the analysis.
                        level 0: run application and generate reports and traces only
                        level 1: print analysis of report and trace data (default)
                        level 2: create plots from report and trace data
  -s, --skip_launch     do not launch jobs, only analyze existing data
  --version             show the GEOPM version number and exit

""".format(argv_0=sys.argv[0])
    version_str = """\
GEOPM version {version}
Copyright (C) 2015, 2016, 2017, Intel Corporation. All rights reserved.
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

    parser.add_argument('-t', '--analysis_type',
                        action='store', required=True, default='REQUIRED OPTION')
    parser.add_argument('-n', '--num_rank',
                        action='store', required=True, default=None, type=int)
    parser.add_argument('-N', '--num_node',
                        action='store', required=True, default=None, type=int)
    parser.add_argument('-o', '--output_dir',
                        action='store', default='.')
    parser.add_argument('-p', '--profile_prefix',
                        action='store', default='')
    parser.add_argument('-l', '--level',
                        action='store', default=1, type=int)
    parser.add_argument('app_argv', metavar='APP_ARGV',
                        action='store', nargs='*')
    parser.add_argument('-s', '--skip_launch',
                        action='store_true', default=False)

    args = parser.parse_args(argv)
    if args.analysis_type not in analysis_type_map:
        raise SyntaxError('Analysis type: "{}" unrecognized.'.format(args.analysis_type))

    if False:
        if not args.profile_prefix:
            raise RuntimeError('profile_prefix is required to skip launch.')

    analysis = analysis_type_map[args.analysis_type](args.profile_prefix,
                                                     args.output_dir,
                                                     args.num_rank,
                                                     args.num_node,
                                                     args.app_argv)

    if args.skip_launch:
        analysis.find_files()
    else:
        # if launching, must run within an allocation to make sure all runs use
        # the same set of nodes
        if 'SLURM_NNODES' in os.environ:
             num_node = os.getenv('SLURM_NNODES')
        elif 'COBALT_NODEFILE' in os.environ:
             with open(os.getenv('COBALT_NODEFILE')) as fid:
                 num_node = len(fid.readlines())
        else:
            num_node = -1
        if num_node != args.num_node:
            raise RuntimeError('Launch must be made inside of a job allocation and application must run on all allocated nodes.')

        analysis.launch()

    if args.level > 0:
        parse_output = analysis.parse()
        process_output = analysis.report_process(parse_output)
        analysis.report(process_output)
        if args.level > 1:
            process_output = analysis.plot_process(parse_output)
            analysis.plot(process_output)
