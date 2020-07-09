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

"""POWER_BALANCER

"""

import sys
import unittest
import os
import glob
import pandas

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from integration.test import geopm_context
import geopmpy.io
import geopmpy.error

from integration.test import util
if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    from integration.test import geopm_test_launcher
    geopmpy.error.exc_clear()


@util.skip_unless_run_long_tests()
@util.skip_unless_batch()
class TestIntegration_power_balancer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_power_balancer'
        cls._num_node = 4
        cls._agent_list = ['power_governor', 'power_balancer']
        cls._skip_launch = not util.do_launch()
        cls._tmp_files = []
        cls._keep_files = (cls._skip_launch or
                           os.getenv('GEOPM_KEEP_FILES') is not None)

        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()

        if not cls._skip_launch:
            num_rank = 16
            loop_count = 500
            fam, mod = geopm_test_launcher.get_platform()
            power_budget = 200
            if fam == 6 and mod == 87:
                # budget for KNL
                power_budget = 130
            options = {'power_budget': power_budget}
            gov_agent_conf_path = cls._test_name + '_gov_agent.config'
            bal_agent_conf_path = cls._test_name + '_bal_agent.config'
            cls._tmp_files.append(gov_agent_conf_path)
            # self._tmp_files.append(bal_agent_conf_path)
            path_dict = {'power_governor': gov_agent_conf_path, 'power_balancer': bal_agent_conf_path}

            app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
            # self._tmp_files.append(app_conf.get_path())
            app_conf.append_region('dgemm-imbalance', 8.0)
            app_conf.append_region('all2all', 0.05)
            app_conf.set_loop_count(loop_count)
            # Update app config with imbalance
            alloc_nodes = geopm_test_launcher.TestLauncher.get_alloc_nodes()
            for nn in range(len(alloc_nodes) // 2):
                app_conf.append_imbalance(alloc_nodes[nn], 0.5)

            for agent in cls._agent_list:
                agent_conf = geopmpy.io.AgentConf(path_dict[agent], agent, options)
                run_name = '{}_{}'.format(cls._test_name, agent)
                report_path = '{}.report'.format(run_name)
                trace_path = '{}.trace'.format(run_name)
                cls._tmp_files.append(report_path)
                cls._tmp_files.append(trace_path)
                launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path,
                                                            trace_path, time_limit=2700)
                launcher.set_num_node(cls._num_node)
                launcher.set_num_rank(num_rank)
                launcher.write_log(run_name, 'Power cap = {}W'.format(power_budget))
                launcher.run(run_name)

    @classmethod
    def tearDownClass(cls):
        """Clean up any files that may have been created during the test if we
        are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset.

        """
        if not cls._keep_files:
            for path in cls._tmp_files:
                for tf in glob.glob(path + '.*'):
                    os.unlink(tf)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_power_balancer._keep_files = True

    def test_power_balancer(self):

        # Require that the balancer moves the maximum dgemm runtime at
        # least 1/4 the distance to the mean dgemm runtime under the
        # governor.
        margin_factor = 0.25
        agent_runtime = dict()
        for agent in self._agent_list:
            # TODO: these lines repeated above
            run_name = '{}_{}'.format(self._test_name, agent)
            report_path = '{}.report'.format(run_name)
            trace_path = '{}.trace'.format(run_name)
            #

            output = geopmpy.io.AppOutput(report_path, trace_path + '*')
            node_names = output.get_node_names()
            self.assertEqual(self._num_node, len(node_names))

            new_output = geopmpy.io.RawReport(report_path)
            print(new_output.meta_data())
            power_budget = new_output.meta_data()['Policy']['POWER_PACKAGE_LIMIT_TOTAL']

            power_limits = []
            # Total power consumed will be Socket(s) + DRAM
            for nn in node_names:
                tt = output.get_trace_data(node_name=nn)

                first_epoch_index = tt.loc[tt['EPOCH_COUNT'] == 0][:1].index[0]
                epoch_dropped_data = tt[first_epoch_index:]  # Drop all startup data

                power_data = epoch_dropped_data.filter(regex='ENERGY')
                power_data['TIME'] = epoch_dropped_data['TIME']
                power_data = power_data.diff().dropna()
                power_data.rename(columns={'TIME': 'ELAPSED_TIME'}, inplace=True)
                power_data = power_data.loc[(power_data != 0).all(axis=1)]  # Will drop any row that is all 0's

                pkg_energy_cols = [s for s in power_data.keys() if 'ENERGY_PACKAGE' in s]
                dram_energy_cols = [s for s in power_data.keys() if 'ENERGY_DRAM' in s]
                power_data['SOCKET_POWER'] = power_data[pkg_energy_cols].sum(axis=1) / power_data['ELAPSED_TIME']
                power_data['DRAM_POWER'] = power_data[dram_energy_cols].sum(axis=1) / power_data['ELAPSED_TIME']
                power_data['COMBINED_POWER'] = power_data['SOCKET_POWER'] + power_data['DRAM_POWER']

                pandas.set_option('display.width', 100)
                # launcher.write_log(name, 'Power stats from {} {} :\n{}'.format(agent, nn, power_data.describe()))
                # TODO: still want this in launcher log?
                sys.stdout.write('Power stats from {} {} :\n{}\n'.format(agent, nn, power_data.describe()))

                # Get final power limit set on the node
                if agent == 'power_balancer':
                    power_limits.append(epoch_dropped_data['POWER_LIMIT'][-1])

            if agent == 'power_balancer':
                avg_power_limit = sum(power_limits) / len(power_limits)
                self.assertTrue(avg_power_limit <= power_budget)

            max_runtime = float('nan')
            node_names = output.get_node_names()
            runtime_list = []
            for node_name in node_names:
                epoch_data = output.get_report_data(node_name=node_name, region='dgemm')
                runtime_list.append(epoch_data['runtime'].item())
            if agent == 'power_governor':
                mean_runtime = sum(runtime_list) / len(runtime_list)
                max_runtime = max(runtime_list)
                margin = margin_factor * (max_runtime - mean_runtime)

            agent_runtime[agent] = max(runtime_list)

        sys.stdout.write("\nAverage runtime stats:\n")
        sys.stdout.write("governor runtime: {}, balancer runtime: {}, margin: {}\n".format(
            agent_runtime['power_governor'], agent_runtime['power_balancer'], margin))

        self.assertGreater(agent_runtime['power_governor'] - margin,
                           agent_runtime['power_balancer'],
                           "governor runtime: {}, balancer runtime: {}, margin: {}".format(
                               agent_runtime['power_governor'], agent_runtime['power_balancer'], margin))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
