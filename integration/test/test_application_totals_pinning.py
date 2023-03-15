#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
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
from integration.test import geopm_test_launcher


# This skip decorator "False" is required to prevent this test from being run in the nightlies.
# This test always fails under normal operation, and we want to prevent this test from running
# during the nightlies, because any single failed test would fail the nightlies, and we don't
# want that. While a fix is pending, the expectedFailure decorator is used to assert the test
# fails as expected.
@unittest.skipUnless(geopm_test_launcher.detect_launcher() == "srun",
                     'Using srun --cpu-bind command line option in this test')
@unittest.expectedFailure
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
        cls._num_node = 1
        num_rank = 1
        time_limit = 6000
        # Configure the test application
        cls._spin_bigo = 0.5
        cls._sleep_bigo = 1.0
        cls._unmarked_bigo = 1.0
        cls._loop_count = 4
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
                                                    time_limit=time_limit)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(test_name, add_geopm_args=['--geopm-affinity-disable',
                                                '--cpu-bind=map_cpu=0',
                                                '--geopm-report-signals=CPU_ENERGY@package'])

        # Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def test_package_energy(self):
        self._report = geopmpy.io.RawReport(self._report_path)
        node_names = self._report.host_names()
        for nn in node_names:
            report = self._report.raw_totals(host_name=nn)
            package_energy_0 = report['CPU_ENERGY@package-0']
            package_energy_1 = report['CPU_ENERGY@package-1']
            self.assertNotEqual(0, package_energy_0)
            self.assertNotEqual(0, package_energy_1)


if __name__ == '__main__':
    unittest.main()
