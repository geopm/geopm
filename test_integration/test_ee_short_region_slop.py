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

"""

import sys
import unittest
import os
import glob
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import pandas

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import geopmpy.io

_g_skip_launch = False
try:
    sys.argv.remove('--skip-launch')
    _g_skip_launch = True
except ValueError:
    from test_integration import geopm_context
    from test_integration import geopm_test_launcher
    from test_integration import util
    if 'exc_clear' in dir(sys):
        sys.exc_clear()

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
        cls._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None
        cls._agent_conf_fixed_path = 'test_{}_fixed-agent-config.json'.format(test_name)
        cls._agent_conf_dynamic_path = 'test_{}_dynamic-agent-config.json'.format(test_name)
        # Clear out exception record for python 2 support
        if 'exc_clear' in dir(sys):
            sys.exc_clear()
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

            # Create the test launcher with the above configuration
            launcher_fixed = geopm_test_launcher.TestLauncher(app_conf,
                                                              agent_conf_fixed,
                                                              cls._report_path_fixed,
                                                              time_limit=time_limit)
            launcher_fixed.set_num_node(num_node)
            launcher_fixed.set_num_rank(num_rank)
            # Run the test application
            launcher_fixed.run(test_name)

            # Create the test launcher with the above configuration
            launcher_dynamic = geopm_test_launcher.TestLauncher(app_conf,
                                                                agent_conf_dynamic,
                                                                cls._report_path_dynamic,
                                                                time_limit=time_limit)
            launcher_dynamic.set_num_node(num_node)
            launcher_dynamic.set_num_rank(num_rank)
            # Run the test application
            launcher_dynamic.run(test_name)

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
            os.unlink(cls._report_path_fixed)
            os.unlink(cls._report_path_dynamic)
            os.unlink(cls._image_path)

    def test_generate_plot(self):
        """Visualize the data in the report

        """
        report_dynamic = geopmpy.io.RawReport(self._report_path_dynamic)
        cols, scaling_data_dynamic, timed_data_dynamic = extract_data(report_dynamic)

        report_fixed = geopmpy.io.RawReport(self._report_path_fixed)
        cols, scaling_data_fixed, timed_data_fixed = extract_data(report_fixed)

        all_energy = list(scaling_data_fixed[column_idx(cols, 'package-energy')])
        all_energy.extend(scaling_data_dynamic[column_idx(cols, 'package-energy')])
        all_energy.extend(timed_data_fixed[column_idx(cols, 'package-energy')])
        all_energy.extend(timed_data_dynamic[column_idx(cols, 'package-energy')])
        min_energy = min(all_energy)
        max_energy = max(all_energy)
        delta_energy = max_energy - min_energy
        min_energy -= 0.05 * delta_energy
        max_energy += 0.05 * delta_energy

        all_runtime = list(scaling_data_fixed[column_idx(cols, 'runtime')])
        all_runtime.extend(scaling_data_dynamic[column_idx(cols, 'runtime')])
        all_runtime.extend(timed_data_fixed[column_idx(cols, 'runtime')])
        all_runtime.extend(timed_data_dynamic[column_idx(cols, 'runtime')])
        min_runtime = min(all_runtime)
        max_runtime = max(all_runtime)
        delta_runtime = max_runtime - min_runtime
        min_runtime -= 0.05 * delta_runtime
        max_runtime += 0.05 * delta_runtime

        plt.figure(figsize=(11,11))
        plt.subplot(2, 2, 1)
        plt.title('Scaling region package-energy')
        plot_data(cols, scaling_data_fixed, 'duration', 'package-energy')
        plot_data(cols, scaling_data_dynamic, 'duration', 'package-energy')
        plt.ylim(min_energy, max_energy)
        plt.legend(('fixed', 'dynamic'))

        plt.subplot(2, 2, 2)
        plt.title('Timed region package-energy')
        plot_data(cols, timed_data_fixed, 'duration', 'package-energy')
        plot_data(cols, timed_data_dynamic, 'duration', 'package-energy')
        plt.ylim(min_energy, max_energy)
        plt.legend(('fixed', 'dynamic'))

        plt.subplot(2, 2, 3)
        plt.title('Scaling region runtime')
        plot_data(cols, scaling_data_fixed, 'duration', 'runtime')
        plot_data(cols, scaling_data_dynamic, 'duration', 'runtime')
        plt.ylim(min_runtime, max_runtime)
        plt.legend(('fixed', 'dynamic'))

        plt.subplot(2, 2, 4)
        plt.title('Timed region runtime')
        plot_data(cols, timed_data_fixed, 'duration', 'runtime')
        plot_data(cols, timed_data_dynamic, 'duration', 'runtime')
        plt.ylim(min_runtime, max_runtime)
        plt.legend(('fixed', 'dynamic'))
        plt.savefig(self._image_path)

def extract_data(report):
    cols = [('count', ''),
            ('package-energy', 'joules'),
            ('requested-online-frequency', ''),
            ('power', 'watts'),
            ('runtime', 'sec'),
            ('frequency', 'Hz')]
    scaling_data = extract_data_region(report, cols, 'scaling')
    timed_data = extract_data_region(report, cols, 'timed')
    cols.append(('duration', 'sec'))
    dur = tuple(rt / ct for ct, rt in zip(scaling_data[column_idx(cols, 'count')],
                                          scaling_data[column_idx(cols, 'runtime')]))
    scaling_data.append(dur)
    dur = tuple(rt / ct for ct, rt in zip(timed_data[column_idx(cols, 'count')],
                                          timed_data[column_idx(cols, 'runtime')]))
    timed_data.append(dur)
    return cols, scaling_data, timed_data

def column_idx(cols, name):
    return zip(*cols)[0].index(name)

def plot_data(cols, data, xaxis, yaxis):
    xaxis_idx = column_idx(cols, xaxis)
    yaxis_idx = column_idx(cols, yaxis)
    plt.semilogx(data[xaxis_idx], data[yaxis_idx], 'x')
    units = cols[xaxis_idx][1]
    if units:
        plt.xlabel('{} ({})'.format(xaxis, units))
    else:
        plt.xlabel(xaxis)
    units = cols[yaxis_idx][1]
    if units:
        plt.ylabel('{} ({})'.format(yaxis, units))
    else:
        plt.ylabel(yaxis)

def extract_data_region(report, cols, key):
    host = report.host_names()[0]
    region_names = report.region_names(host)
    regions = [report.raw_region(host, rn)
               for rn in region_names
               if rn.startswith(key)]
    result = []
    for rr in regions:
         sample = []
         for cc in cols:
             try:
                 sample.append(report.get_field(rr, cc[0], cc[1]))
             except KeyError:
                 sample.append(None)
         result.append(sample)
    return zip(*result)

if __name__ == '__main__':
    unittest.main()
