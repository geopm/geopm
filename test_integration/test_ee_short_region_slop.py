#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

try:
    import geopmpy.io
    import geopmpy.error
except ImportError:
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from test_integration import geopm_context
    import geopmpy.io
    import geopmpy.error

_g_skip_launch = False
try:
    sys.argv.remove('--skip-launch')
    _g_skip_launch = True
except ValueError:
    from test_integration import geopm_test_launcher
    from test_integration import util
    geopmpy.error.exc_clear()

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
        test_name = 'ee_short_region_slop'
        cls._report_path_fixed = 'test_{}_fixed.report'.format(test_name)
        cls._report_path_dynamic = 'test_{}_dynamic.report'.format(test_name)
        cls._image_path = 'test_{}.png'.format(test_name)
        cls._skip_launch = _g_skip_launch
        cls._keep_files = cls._skip_launch or os.getenv('GEOPM_KEEP_FILES') is not None
        cls._agent_conf_fixed_path = 'test_{}_fixed-agent-config.json'.format(test_name)
        cls._agent_conf_dynamic_path = 'test_{}_dynamic-agent-config.json'.format(test_name)
        cls._num_trial = 40
        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()
        if not cls._skip_launch:
            # Set the job size parameters
            num_node = 1
            num_rank = 1
            time_limit = 6000
            # Configure the test application
            app_conf = AppConf()

            # Region hashes for scaling_0, scaling_1, ... , scaling_11
            scaling_rid = [0x8a244fc3, 0xc31832e4, 0x185cb58d, 0x5160c8aa,
                           0xab39cdae, 0xe205b089, 0x394137e0, 0x707d4ac7,
                           0xc81f4b19, 0x8123363e, 0x62184bff, 0x0a1b6737]
            # Region hashes for timed_0, timed_1, ... , timed_11
            timed_rid = [0x2dbeb83f, 0x3e1c2048, 0x0afb88d1, 0x195910a6,
                         0x6334d9e3, 0x70964194, 0x4471e90d, 0x57d3717a,
                         0xb0aa7b87, 0xa308e3f0, 0x0eff69f9, 0xfc94eafa]
            # Configure the agent
            # Query for the min and sticker frequency and run the
            # frequency map agent over this range.
            freq_min = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
            freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
            agent_conf_fixed_dict = {'FREQ_MIN':freq_min,
                                     'FREQ_MAX':freq_sticker}
            agent_conf_fixed = geopmpy.io.AgentConf(cls._agent_conf_fixed_path,
                                                    'frequency_map',
                                                    agent_conf_fixed_dict)
            agent_conf_dynamic_dict = dict(agent_conf_fixed_dict)
            policy_idx = 0
            for rid in scaling_rid:
                agent_conf_dynamic_dict['HASH_{}'.format(policy_idx)] = rid
                agent_conf_dynamic_dict['FREQ_{}'.format(policy_idx)] = freq_sticker
                policy_idx += 1
            for rid in timed_rid:
                agent_conf_dynamic_dict['HASH_{}'.format(policy_idx)] = rid
                agent_conf_dynamic_dict['FREQ_{}'.format(policy_idx)] = freq_min
                policy_idx += 1

            agent_conf_dynamic = geopmpy.io.AgentConf(cls._agent_conf_dynamic_path,
                                                      'frequency_map',
                                                      agent_conf_dynamic_dict)

            for trial_idx in range(cls._num_trial):
                sys.stdout.write('\nTrial {} / {}\n'.format(trial_idx, cls._num_trial))
                path = '{}.{}'.format(cls._report_path_fixed, trial_idx)
                # Create the test launcher with the above configuration
                launcher_fixed = geopm_test_launcher.TestLauncher(app_conf,
                                                                  agent_conf_fixed,
                                                                  path,
                                                                  time_limit=time_limit)
                launcher_fixed.set_num_node(num_node)
                launcher_fixed.set_num_rank(num_rank)
                # Run the test application
                launcher_fixed.run('{}-fixed'.format(test_name))
                path = '{}.{}'.format(cls._report_path_dynamic, trial_idx)
                # Create the test launcher with the above configuration
                launcher_dynamic = geopm_test_launcher.TestLauncher(app_conf,
                                                                    agent_conf_dynamic,
                                                                    path,
                                                                    time_limit=time_limit)
                launcher_dynamic.set_num_node(num_node)
                launcher_dynamic.set_num_rank(num_rank)
                # Run the test application
                launcher_dynamic.run('{}-dynamic'.format(test_name))


    @classmethod
    def tearDownClass(cls):
        """Clean up any files that may have been created during the test if we
        are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset.

        """

        if (sys.exc_info() == (None, None, None) and not
            cls._keep_files and not
            cls._skip_launch):
            os.unlink(cls._agent_conf_fixed_path)
            os.unlink(cls._agent_conf_dynamic_path)
            for trial_idx in range(cls._num_trial):
                rf = '{}.{}'.format(cls._report_path_fixed, trial_idx)
                os.unlink(rf)
                rf = '{}.{}'.format(cls._report_path_dynamic, trial_idx)
                os.unlink(rf)
            os.unlink(cls._image_path)


    def test_generate_plot(self):
        """Visualize the data in the report

        """
        data = pandas.DataFrame()
        for trial_idx in range(self._num_trial):
            try:
                report_path = '{}.{}'.format(self._report_path_fixed, trial_idx)
                report_fixed = geopmpy.io.RawReport(report_path)
                report_path = '{}.{}'.format(self._report_path_dynamic, trial_idx)
                report_dynamic = geopmpy.io.RawReport(report_path)
                data = data.append(extract_data(report_fixed, trial_idx))
                data = data.append(extract_data(report_dynamic, trial_idx))
            except:
                if trial_idx == 0:
                    raise RuntimeError('Error: No reports found')
                sys.stderr.write('Warning: only {} out of {} report files have been loaded.\n'.format(trial_idx, self._num_trial))
                break

        yaxis_top = 'package-energy (joules)'

        # Uncomment to make frequency plot
        # yaxis_top = 'frequency (Hz)'

        # Set index for data selection
        data = data.set_index(['profile-name', 'region-name', 'count'])
        plt.figure(figsize=(11,11))

        # First subplot
        ax = plt.subplot(2, 2, 1)
        ax.set_xscale("log")
        plot_data(data, 'scaling', 'duration (sec)', yaxis_top)
        ylim = list(plt.ylim())

        ax = plt.subplot(2, 2, 2)
        ax.set_xscale("log")
        plot_data(data, 'timed', 'duration (sec)', yaxis_top)
        # Align y axis limits for top plots
        yl = plt.ylim()
        ylim[0] = min(yl[0], ylim[0])
        ylim[1] = max(yl[1], ylim[1])
        plt.ylim(ylim)
        plt.subplot(2, 2, 1)
        plt.ylim(ylim)

        ax = plt.subplot(2, 2, 3)
        ax.set_xscale("log")
        plot_data(data, 'scaling', 'duration (sec)', 'runtime (sec)')
        ylim = list(plt.ylim())

        ax = plt.subplot(2, 2, 4)
        ax.set_xscale("log")
        plot_data(data, 'timed', 'duration (sec)', 'runtime (sec)')
        # Align y axis limits for bottom plots
        yl = plt.ylim()
        ylim[0] = min(yl[0], ylim[0])
        ylim[1] = max(yl[1], ylim[1])
        plt.ylim(ylim)
        plt.subplot(2, 2, 3)
        plt.ylim(ylim)

        plt.savefig(self._image_path)


def extract_data(report, trial_idx):
    cols = ['count',
            'package-energy (joules)',
            'requested-online-frequency',
            'power (watts)',
            'runtime (sec)',
            'frequency (Hz)']

    # Extract data frame for regions
    scaling_data = extract_data_region(report, cols, 'scaling')
    timed_data = extract_data_region(report, cols, 'timed')
    dur = scaling_data['runtime (sec)'] / scaling_data['count']
    scaling_data['duration (sec)'] = dur

    dur = timed_data['runtime (sec)'] / timed_data['count']
    timed_data['duration (sec)'] = dur
    prof_name = report.meta_data()['Profile']
    scaling_data['profile-name'] = prof_name
    timed_data['profile-name'] = prof_name
    scaling_data['trial-idx'] = trial_idx
    timed_data['trial-idx'] = trial_idx
    return scaling_data.append(timed_data)


def plot_data(data, region_type, xaxis, yaxis):
    for policy_type in ('fixed', 'dynamic'):
        prof_name = 'ee_short_region_slop-{}'.format(policy_type)
        key = (prof_name, region_type)
        level = ('profile-name', 'region-name')
        selected_data = data.xs(key=key, level=level).groupby('count')
        xdata = selected_data.mean()[xaxis]
        ydata = selected_data.mean()[yaxis]
        ydata_std = selected_data.std()[yaxis]
        plt.errorbar(xdata, ydata, ydata_std, fmt='.')
    plt.title('{} region {}'.format(region_type, yaxis))
    plt.xlabel(xaxis)
    plt.ylabel(yaxis)
    plt.legend(('fixed', 'dynamic'))


def extract_data_region(report, cols, key):
    host = report.host_names()[0]
    region_names = report.region_names(host)
    regions = [(rn, report.raw_region(host, rn))
               for rn in region_names
               if rn.startswith(key)]
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
                 sample.append(report.get_field(rr, cc))
             except KeyError:
                 sample.append(None)
        result[cc] = sample
    return pandas.DataFrame(result)

if __name__ == '__main__':
    unittest.main()
