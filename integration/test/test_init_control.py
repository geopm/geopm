#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import unittest
import os

import geopmpy.io
import geopmpy.agent
import geopmdpy.error
from integration.test import util
from integration.test import geopm_test_launcher


class TestIntegrationInitControl(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_init_control'
        cls._report_path = '{}.report'.format(test_name)
        cls._trace_path = '{}.trace'.format(test_name)
        cls._agent_conf_path = 'test_' + test_name + '-agent-config.json'
        # Set the job size parameters
        cls._num_node = util.get_num_node()
        num_rank = 2 * cls._num_node
        time_limit = 600
        # Configure the test application
        cls._loop_count = 500
        app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
        app_conf.set_loop_count(cls._loop_count)
        app_conf.append_region('dgemm', 8.0)

        # Configure the monitor agent
        agent_conf = geopmpy.agent.AgentConf(test_name + '_agent.config')

        # Create the InitControl configuration
        min_power = geopm_test_launcher.geopmread("CPU_POWER_MIN_AVAIL board 0")
        cls._requested_power_limit = min_power + 40
        cls._requested_time_window = 0.013671875 # 7 bit float representation of 0.015
        init_control_path = 'init_control'
        with open('init_control', 'w') as outfile:
            outfile.write(f'CPU_POWER_LIMIT_CONTROL board 0 {cls._requested_power_limit} '
                           '# Sets a power limit\n'
                           '# Next we\'ll set the time window:\n'
                          f'CPU_POWER_TIME_WINDOW_CONTROL board 0 {cls._requested_time_window}\n')

        # Capture trace signals for the desired controls
        trace_signals = 'CPU_POWER_LIMIT_CONTROL@board,CPU_POWER_TIME_WINDOW_CONTROL@board'

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path,
                                                    time_limit=time_limit,
                                                    init_control_path=init_control_path,
                                                    trace_signals=trace_signals)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(test_name)

        # Output to be reused by all tests
        cls._trace_output = geopmpy.io.AppOutput(traces=cls._trace_path + '*')
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def test_controls(self):
        '''Test desired controls were applied successfully
        '''
        for node in self._node_names:
            trace_data = self._trace_output.get_trace_data(node_name=node)
            dgemm_data = self._report.raw_region(node, 'dgemm')
            epoch_data = self._report.raw_epoch(node)

            # Verify achieved power is in the ballpark of the requested limit
            self.assertAlmostEqual(self._requested_power_limit, dgemm_data['power (W)'],
                                   delta=self._requested_power_limit * 0.02)

            # Use the trace data to verify that the controls were set absolutely
            self.assertEqual(trace_data['CPU_POWER_LIMIT_CONTROL'].iloc[0],
                             self._requested_power_limit)
            self.assertEqual(trace_data['CPU_POWER_LIMIT_CONTROL'].std(), 0.0)

            self.assertEqual(trace_data['CPU_POWER_TIME_WINDOW_CONTROL'].iloc[0],
                             self._requested_time_window)
            self.assertEqual(trace_data['CPU_POWER_TIME_WINDOW_CONTROL'].std(), 0.0)


if __name__ == '__main__':
    unittest.main()
