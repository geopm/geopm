#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the gpu_activity agent can improve
efficiency of an application.
"""
import sys
import unittest
import os

import geopmpy.io
import geopmpy.agent
import geopmdpy.error
from integration.test import geopm_test_launcher
from integration.test import util


class AppConf(object):
    """Class that is used by the test launcher to get access
       to the test_omp_outer_loop test binary..

    """
    def write(self):
        """No configuration files are required.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_ompt_gpu')

    def get_exec_args(self):
        return []


@util.skip_unless_do_launch()
@util.skip_unless_gpu()
class TestIntegration_ompt_gpu(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_ompt_gpu'
        cls._expected_regions = ["[OMPT]__tgt_target_teams_mapper+0x0", "[OMPT]__tgt_target_mapper+0x0"]
        cls._report_path = []
        cls._skip_launch = not util.do_launch()
        num_node = 1
        num_rank = 1
        curr_run = test_name
        report_path = curr_run + '.report'
        cls._report_path.append(report_path)
        cls._agent_conf_path = test_name + '-agent-config.json'
        if not cls._skip_launch:
            agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path)
            launcher = geopm_test_launcher.TestLauncher(AppConf(),
                                                        agent_conf,
                                                        report_path,
                                                        time_limit=6000)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            launcher.run(curr_run)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_ompt_gpu._keep_files = True

    def test_offload_regions(self):
        """
        """
        print("test")
        report = geopmpy.io.RawReport(self._report_path[0])
        host_names = report.host_names()
        for host_name in report.host_names():
            for expected_region in self._expected_regions:
                self.assertTrue(expected_region in report.region_names(host_name),
                                 msg='Region {} should be present in report'.format(expected_region))
        #TODO: check that GPU_UTILIZATION is non-zero
        #TODO: check count is 10 (hard-coded in test_ompt_gpu.cpp)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
