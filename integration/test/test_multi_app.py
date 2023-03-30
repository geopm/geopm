#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""TODO: add high-level description of test
"""

import sys
import unittest
import os

import geopmpy.agent
import geopmpy.io
from integration.test import geopm_test_launcher


class AppConf(object):
    """Custom AppConf needed by test to pass basic app configuration to
    TestLauncher
    """
    def write(self):
        """Called by TestLauncher to generate any configuration files app might
        need. No file is generated for this test.
        """
        pass

    def get_exec_path(self):
        """Path to the test application.
        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, 'test_multi_app.sh')

    def get_exec_args(self):
        """Returns a list of strings representing the command-line arguments to
        pass to the test application.
        """
        return []


class TestIntegration_multi_app(unittest.TestCase):
    TEST_NAME = 'test_multi_app'
    TIME_LIMIT = 6000  # TODO: what time limit should test use? Use default?
    NUM_NODE = 1
    NUM_RANK = 3

    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._report_path = f'{cls.TEST_NAME}.report'
        cls._trace_path = f'{cls.TEST_NAME}.trace'

        # Create app config for test application
        app_conf = AppConf()
        # Create agent config (use default monitor agent)
        agent_conf = geopmpy.agent.AgentConf(cls.TEST_NAME + '_agent.config')
        # Create and configure launcher
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path,
                                                    time_limit=cls.TIME_LIMIT)
        launcher.set_num_node(cls.NUM_NODE)
        launcher.set_num_rank(cls.NUM_RANK)
        launcher.set_pmpi_ctl('application')

        launcher.run(cls.TEST_NAME)

        # Run output to be consumed by tests
        cls._trace_output = geopmpy.io.AppOutput(traces=cls._trace_path + '*')
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def test_temp_debug(self):
        self.assertTrue(os.path.exists(self._report_path))
        self.assertIsNotNone(self._node_names)
        for node in self._node_names:
            trace_data = self._trace_output.get_trace_data(node_name=node)
            self.assertIsNotNone(trace_data)


if __name__ == '__main__':
    unittest.main()
