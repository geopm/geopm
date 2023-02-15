#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import unittest
import os

import geopmpy.agent
import geopmpy.io
import geopmdpy.error
from integration.test import geopm_test_launcher
from integration.test import util


class TestIntegration_launch_application(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_launch_application'
        cls._report_path = '{}.report'.format(cls._test_name)
        cls._trace_path_prefix = '{}_trace'.format(cls._test_name)
        cls._agent_conf_path = cls._test_name + '-agent-config.json'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        app_conf.append_region('sleep', 1.0)
        agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path)
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path_prefix)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.set_pmpi_ctl('application')
        launcher.run(cls._test_name)

    @unittest.skipUnless(geopm_test_launcher.detect_launcher() != "aprun",
                         'ALPS does not support multi-application launch on the same nodes.')
    @util.skip_unless_batch()
    def test_report_and_trace_generation_application(self):
        '''Test that GEOPM can be launched with the controller in a background
           process.

        '''

        self._trace = geopmpy.io.AppOutput(traces=self._trace_path_prefix + '*')
        self._report = geopmpy.io.RawReport(self._report_path)
        node_names = self._report.host_names()
        for nn in node_names:
            report = self._report.raw_totals(host_name=nn)
            self.assertNotEqual(0, len(report))
            trace = self._trace.get_trace_data(node_name=nn)
            self.assertNotEqual(0, len(trace))


if __name__ == '__main__':
    unittest.main()
