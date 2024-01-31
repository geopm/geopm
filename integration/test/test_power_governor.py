#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import unittest
import os
import pandas

import geopmpy.io
import geopmpy.agent

from integration.test import util
from integration.test import geopm_test_launcher


class TestIntegration_power_governor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_power_governor'
        cls._report_path = '{}.report'.format(cls._test_name)
        cls._trace_path = '{}.trace'.format(cls._test_name)
        cls._agent_conf_path = cls._test_name + '-agent-config.json'
        # Set the job size parameters
        cls._num_node = util.get_num_node()
        num_rank = 4 * cls._num_node
        loop_count = 500
        app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        app_conf.append_region('dgemm', 8.0)
        app_conf.set_loop_count(loop_count)

        cls._agent = 'power_governor'
        cls._options = dict()
        min_power = geopm_test_launcher.geopmread("CPU_POWER_MIN_AVAIL board 0")
        cls._options['power_budget'] = min_power + 40
        agent_conf = geopmpy.agent.AgentConf(cls._test_name + '_agent.config', cls._agent, cls._options)

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(cls._test_name)

        # Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._trace = geopmpy.io.AppOutput(cls._trace_path + '*')
        cls._node_names = cls._report.host_names()

    def test_power_consumption(self):
        self.assertEqual(self._num_node, len(self._node_names))
        all_power_data = dict()
        for nn in self._node_names:
            tt = self._trace.get_trace_data(node_name=nn)

            first_epoch_index = tt.loc[tt['EPOCH_COUNT'] == 0][:1].index[0]
            epoch_dropped_data = tt[first_epoch_index:]  # Drop all startup data
            pandas.set_option('display.width', 100)
            with open('{}.log'.format(self._test_name), 'a') as fid:
                fid.write('\n{}: Power stats from host {}: \n{}\n\n'.format(
                    self._test_name, nn, epoch_dropped_data['CPU_POWER'].describe()))
            # Allow for overages of 2% at the 75th percentile.
            self.assertGreater(self._options['power_budget'] * 1.02, epoch_dropped_data['CPU_POWER'].quantile(.75))

            # TODO Checks on the maximum power computed during the run?
            # TODO Checks to see how much power was left on the table?


if __name__ == '__main__':
    unittest.main()
