#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Test the detection of user defined regions in an application
   configured such that the outer loop is an OMP paralell region.

"""


import sys
import re
import unittest
import os
import glob

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
        """Path to bencmark

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_omp_outer_loop')

    def get_exec_args(self):
        return []


class TestIntegrationOMPOuterLoop(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_omp_outer_loop'
        test_config = ['_with_ompt', '_without_ompt']
        cls._expected_regions = ['MPI_Allreduce']
        cls._report_path = []
        cls._skip_launch = not util.do_launch()
        num_node = 1
        num_rank = 4
        for config in test_config:
            curr_run = test_name + config
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
                if config == '_without_ompt':
                    launcher.disable_ompt()
                launcher.run(curr_run)


    def test_regions_absent(self):
        """Test that the first run's report does NOT contain
           MPI_Allreduce region.
        """
        # self._report_path[0] maps to with-ompt config
        report = geopmpy.io.RawReport(self._report_path[0])
        host_names = report.host_names()
        network_time_detected = False
        for host_name in report.host_names():
            for expected_region in self._expected_regions:
                self.assertFalse(expected_region in report.region_names(host_name),
                                 msg='Region {} should not be present in report'.format(expected_region))
            for detected_region in report.region_names(host_name):
                # second omp parallel region should have some network-time associated with it
                # so here we just assert that at least ONE of the regions has the consumed
                # MPI_Allreduce regions network-time
                if detected_region.startswith('[OMPT]'):
                    observed_region = report.raw_region(host_name, detected_region)
                    if observed_region['time-hint-network (s)'] != 0:
                        network_time_detected = True
            self.assertTrue(network_time_detected,
                            msg="There should be some network time assiciated with an OMPT detected region.")

    def test_regions_present(self):
        """Test that the second run's report DOES contain the
           MPIAllreduce region.
        """
        # self._report_path[1] maps to with-ompt config
        report = geopmpy.io.RawReport(self._report_path[1])
        host_names = report.host_names()
        for host_name in report.host_names():
            for expected_region in self._expected_regions:
                self.assertTrue(expected_region in report.region_names(host_name),
                                 msg='Region {} should be present in report'.format(expected_region))
                observed_region = report.raw_region(host_name, expected_region)
                expected_count = 100
                self.assertEqual(expected_count, observed_region['count'],
                                 msg='Region {} should have count {} in report'.format(expected_region, expected_count))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
