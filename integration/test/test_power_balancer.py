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

"""POWER_BALANCER

"""

import sys
import unittest
import os
import glob
import pandas
import time

import geopm_context
import geopmpy.io
import geopmpy.error

import util
if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    import geopm_test_launcher


@util.skip_unless_batch()
class TestIntegration_power_balancer(unittest.TestCase):

    class AppConf(object):
        """Class that is used by the test launcher in place of a
        geopmpy.io.BenchConf when running the power_balancer benchmark.

        """
        def write(self):
            pass

        def get_exec_path(self):
            script_dir = os.path.dirname(os.path.realpath(__file__))
            return os.path.join(script_dir, '.libs', 'test_power_balancer')

        def get_exec_args(self):
            return []

    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        cls._test_name = 'test_power_balancer'
        cls._num_node = 4
        cls._agent_list = ['power_governor', 'power_balancer']
        cls._skip_launch = not util.do_launch()
        cls._show_details = True
        cls._tmp_files = []

        if not cls._skip_launch:
            report_signals='TIME@package,TIME_HINT_NETWORK@package'
            trace_signals='MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT@package'
            loop_count = 500
            fam, mod = geopm_test_launcher.get_platform()
            alloc_nodes = geopm_test_launcher.TestLauncher.get_alloc_nodes()
            num_node = len(alloc_nodes)
            if (len(alloc_nodes) != cls._num_node):
               err_fmt = 'Warning: <test_power_balancer> Allocation size ({}) is different than expected ({})\n'
               sys.stderr.write(err_fmt.format(num_node, cls._num_node))
               cls._num_node = num_node
            num_rank = 2 * cls._num_node
            power_budget = 180
            if fam == 6 and mod == 87:
                # budget for KNL
                power_budget = 130
            options = {'power_budget': power_budget}
            gov_agent_conf_path = cls._test_name + '_gov_agent.config'
            bal_agent_conf_path = cls._test_name + '_bal_agent.config'
            cls._tmp_files.append(gov_agent_conf_path)
            cls._tmp_files.append(bal_agent_conf_path)
            path_dict = {'power_governor': gov_agent_conf_path, 'power_balancer': bal_agent_conf_path}

            for app_name in ['geopmbench', 'socket_imbalance']:
                app_conf = None
                if app_name == 'geopmbench':
                    app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
                    cls._tmp_files.append(app_conf.get_path())
                    app_conf.append_region('dgemm-imbalance', 8.0)
                    app_conf.set_loop_count(loop_count)
                    # Update app config with imbalance
                    for nn in range(len(alloc_nodes) // 2):
                        app_conf.append_imbalance(alloc_nodes[nn], 0.5)
                elif app_name == 'socket_imbalance':
                    app_conf = cls.AppConf()
                else:
                    raise RuntimeError('No application config for app name {}'.format(app_name))
                for agent in cls._agent_list:
                    agent_conf = geopmpy.agent.AgentConf(path_dict[agent], agent, options)
                    run_name = '{}_{}_{}'.format(cls._test_name, agent, app_name)
                    report_path = '{}.report'.format(run_name)
                    trace_path = '{}.trace'.format(run_name)
                    cls._tmp_files.append(report_path)
                    cls._tmp_files.append(trace_path)
                    launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path,
                                                                trace_path, time_limit=2700,
                                                                report_signals=report_signals,
                                                                trace_signals=trace_signals)
                    launcher.set_num_node(cls._num_node)
                    launcher.set_num_rank(num_rank)
                    launcher.write_log(run_name, 'Power cap = {}W'.format(power_budget))
                    launcher.run(run_name)
                    time.sleep(60)


    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_power_balancer._keep_files = True

    def get_power_data(self, app_name, agent, report_path, trace_path):
        output = geopmpy.io.AppOutput(trace_path + '*')

        new_output = geopmpy.io.RawReport(report_path)
        power_budget = new_output.meta_data()['Policy']['POWER_PACKAGE_LIMIT_TOTAL']
        node_names = new_output.host_names()
        self.assertEqual(self._num_node, len(node_names))

        power_limits = []
        # Total power consumed will be Socket(s) + DRAM
        for nn in node_names:
            tt = output.get_trace_data(node_name=nn)

            first_epoch_index = tt.loc[tt['EPOCH_COUNT'] == 0][:1].index[0]
            epoch_dropped_data = tt[first_epoch_index:]  # Drop all startup data

            power_data = epoch_dropped_data[['TIME', 'ENERGY_PACKAGE', 'ENERGY_DRAM']]
            power_data = power_data.diff().dropna()
            power_data.rename(columns={'TIME': 'ELAPSED_TIME'}, inplace=True)
            power_data = power_data.loc[(power_data != 0).all(axis=1)]  # Will drop any row that is all 0's

            pkg_energy_cols = [s for s in power_data.keys() if 'ENERGY_PACKAGE' in s]
            dram_energy_cols = [s for s in power_data.keys() if 'ENERGY_DRAM' in s]
            power_data['SOCKET_POWER'] = power_data[pkg_energy_cols].sum(axis=1) / power_data['ELAPSED_TIME']
            power_data['DRAM_POWER'] = power_data[dram_energy_cols].sum(axis=1) / power_data['ELAPSED_TIME']
            power_data['COMBINED_POWER'] = power_data['SOCKET_POWER'] + power_data['DRAM_POWER']

            pandas.set_option('display.width', 100)
            power_stats = '\nPower stats from {} {} :\n{}\n'.format(agent, nn, power_data.describe())
            if not self._skip_launch:
                run_name = '{}_{}_{}'.format(self._test_name, agent, app_name)
                with open(run_name + '.log', 'a') as outfile:
                    outfile.write(power_stats)
            if self._show_details:
                sys.stdout.write(power_stats)

            # Get final power limit set on the node
            if agent == 'power_balancer':
                power_limits.append(epoch_dropped_data['ENFORCED_POWER_LIMIT'][-1])

        # Check average limit for job is not exceeded
        if agent == 'power_balancer':
            avg_power_limit = sum(power_limits) / len(power_limits)
            self.assertLessEqual(avg_power_limit, power_budget)

        runtime_list = []
        for nn in node_names:
            dgemm_runtime = new_output.raw_region(nn, 'dgemm')['runtime (s)']
            runtime_list.append(dgemm_runtime)
        return runtime_list

    def balancer_test_helper(self, app_name):
        # Require that the balancer moves the maximum dgemm runtime at
        # least 1/4 the distance to the mean dgemm runtime under the
        # governor.
        margin_factor = 0.50
        agent_runtime = dict()
        for agent in self._agent_list:
            run_name = '{}_{}_{}'.format(self._test_name, agent, app_name)
            report_path = '{}.report'.format(run_name)
            trace_path = '{}.trace'.format(run_name)

            runtime_list = self.get_power_data(app_name, agent, report_path, trace_path)
            agent_runtime[agent] = max(runtime_list)

            if agent == 'power_governor':
                mean_runtime = sum(runtime_list) / len(runtime_list)
                max_runtime = max(runtime_list)
                margin = margin_factor * (max_runtime - mean_runtime)

        if self._show_details:
            sys.stdout.write("\nAverage runtime stats:\n")
            sys.stdout.write("governor runtime: {}, balancer runtime: {}, margin: {}\n".format(
                agent_runtime['power_governor'], agent_runtime['power_balancer'], margin))

        self.assertGreater(agent_runtime['power_governor'] - margin,
                           agent_runtime['power_balancer'],
                           "governor runtime: {}, balancer runtime: {}, margin: {}".format(
                               agent_runtime['power_governor'], agent_runtime['power_balancer'], margin))

    def test_power_balancer_geopmbench(self):
        self.balancer_test_helper('geopmbench')

    @unittest.expectedFailure
    def test_power_balancer_socket_imbalance(self):
        self.balancer_test_helper('socket_imbalance')


if __name__ == '__main__':
    unittest.main()
