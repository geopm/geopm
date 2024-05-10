#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Test that basic information expected when running the synthetic
benchmark is reflected in the reports.

"""

import sys
import unittest
import os

import geopmpy.io
import geopmpy.agent
import geopmdpy.error
from integration.test import util
from integration.test import geopm_test_launcher


class TestIntegration_monitor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_monitor'
        cls._report_path = '{}.report'.format(test_name)
        cls._trace_path = '{}.trace'.format(test_name)
        cls._agent_conf_path = 'test_' + test_name + '-agent-config.json'
        # Set the job size parameters
        cls._num_node = util.get_num_node()
        num_rank = 2 * cls._num_node
        time_limit = 6000
        # Configure the test application
        cls._spin_bigo = 0.5
        cls._sleep_bigo = 1.0
        cls._unmarked_bigo = 1.0
        cls._loop_count = 20
        app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
        app_conf.set_loop_count(cls._loop_count)
        app_conf.append_region('spin', cls._spin_bigo)
        app_conf.append_region('sleep', cls._sleep_bigo)
        app_conf.append_region('sleep-unmarked', cls._unmarked_bigo)

        # Configure the monitor agent
        agent_conf = geopmpy.agent.AgentConf(test_name + '_agent.config')

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path,
                                                    time_limit=time_limit)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(test_name)

        # Output to be reused by all tests
        cls._trace_output = geopmpy.io.AppOutput(traces=cls._trace_path + '*')
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def test_meta_data(self):
        self.assertEqual(len(self._node_names), self._num_node)

    def test_count(self):
        '''Test that region and epoch counts in the report match expected
           number of executions.

        '''
        for node in self._node_names:
            trace_data = self._trace_output.get_trace_data(node_name=node)
            spin_data = self._report.raw_region(node, 'spin')
            sleep_data = self._report.raw_region(node, 'sleep')
            epoch_data = self._report.raw_epoch(node)
            self.assertEqual(self._loop_count, spin_data['count'])
            self.assertEqual(self._loop_count, sleep_data['count'])
            self.assertEqual(self._loop_count, epoch_data['count'])
            self.assertEqual(self._loop_count, trace_data['EPOCH_COUNT'][-1])

    def test_runtime(self):
        '''Test that region and application total runtimes match expected
           runtime from benchmark config

        '''
        for node in self._node_names:
            spin_data = self._report.raw_region(node, 'spin')
            sleep_data = self._report.raw_region(node, 'sleep')
            unmarked_data = self._report.raw_unmarked(node)
            app_total = self._report.raw_totals(node)
            util.assertNear(self, self._loop_count * self._spin_bigo, spin_data['runtime (s)'])
            util.assertNear(self, self._loop_count * self._sleep_bigo, sleep_data['runtime (s)'])
            region_total_runtime = (app_total['GEOPM overhead (s)'] +
                                    spin_data['runtime (s)'] +
                                    sleep_data['runtime (s)'] +
                                    unmarked_data['runtime (s)'])

            total_runtime = app_total['runtime (s)'] - app_total['MPI startup (s)']
            util.assertNear(self, region_total_runtime, total_runtime)

    def test_runtime_epoch(self):
        '''Test that region and epoch total runtimes match.'''
        initial_sleep_time = 5.0
        for node in self._node_names:
            spin_data = self._report.raw_region(node, 'spin')
            sleep_data = self._report.raw_region(node, 'sleep')
            unmarked_data = self._report.raw_unmarked(node)
            epoch_data = self._report.raw_epoch(node)
            total_runtime = (spin_data['runtime (s)'] +
                             sleep_data['runtime (s)'] +
                             unmarked_data['runtime (s)'] -
                             initial_sleep_time)
            util.assertNear(self, total_runtime, epoch_data['runtime (s)'])

    def test_epoch_data_valid(self):
        ''' Test that epoch data is consistent.'''
        for node in self._node_names:
            totals = self._report.raw_totals(node)
            epoch = self._report.raw_epoch(node)

            # Epoch has valid data
            self.assertGreater(epoch['runtime (s)'], 0)
            self.assertGreater(epoch['sync-runtime (s)'], 0)
            self.assertGreater(epoch['package-energy (J)'], 0)
            self.assertGreater(epoch['dram-energy (J)'], 0)
            self.assertGreater(epoch['power (W)'], 0)
            self.assertGreater(epoch['frequency (%)'], 0)
            self.assertGreater(epoch['frequency (Hz)'], 0)
            self.assertEqual(epoch['count'], self._loop_count)
            app_total = self._report.raw_totals(node)
            init_time = app_total['GEOPM overhead (s)']
            initial_sleep_time = 5.0
            total_sync_time = epoch['sync-runtime (s)'] + init_time + initial_sleep_time
            util.assertNear(self, total_sync_time, totals['sync-runtime (s)'])
            for signal in ['sync-runtime (s)', 'package-energy (J)', 'dram-energy (J)']:
                self.assertGreater(totals[signal], epoch[signal], msg='signal={}'.format(signal))

            util.assertNear(self, epoch['runtime (s)'], epoch['sync-runtime (s)'])


if __name__ == '__main__':
    unittest.main()
