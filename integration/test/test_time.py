#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""HINT_TIME

"""

import sys
import unittest
import os
import glob

import geopmpy.io
import geopmpy.agent
import geopmdpy.error

from integration.test import util
if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    from integration.test import geopm_test_launcher

from integration.experiment import machine

class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the hint_time benchmark.

    """
    def write(self):
        """Called by the test launcher prior to executing the test application
        to write any files required by the application.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark filled in by template automatically.

        """
        return 'sleep'

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return ['10']


class TestIntegration_time(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'hint_time'
        cls._report_path = 'test_{}.report'.format(cls._test_name)
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_path = 'test_' + cls._test_name + '-agent-config.json'
        machine_file_name = 'test_{}.machine'.format(cls._test_name)
        cls._machine = machine.Machine()
        try:
            cls._machine.load()
        except RuntimeError:
            cls._machine.save()

        if not cls._skip_launch:
            # Set the job size parameters
            cls._num_node = 4
            num_rank = 4
            time_limit = 60
            # Configure the test application
            app_conf = AppConf()
            agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path)

            # Create the test launcher with the above configuration
            launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                        agent_conf,
                                                        cls._report_path,
                                                        cls._test_name + '.trace',
                                                        time_limit=time_limit)
            launcher.set_num_node(cls._num_node)
            launcher.set_num_rank(num_rank)
            launcher.set_pmpi_ctl('application')
            # Run the test application
            launcher.run('test_' + cls._test_name)
        cls._report = geopmpy.io.RawReport(cls._report_path)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_hint_time._keep_files = True

    def test_hint_time(self):
        host_names = self._report.host_names()
        self.assertEqual(len(host_names), self._num_node)
        for host in host_names:
            found_nw = False
            found_nw_mem = False
            self.assertEqual(0, len(self._report.region_names(host)))
            raw_totals = self._report.raw_totals(host_name=host)
            msg = "Application totals should have 10 seconds of total time"
            expect = 10.0
            actual = raw_totals['runtime (s)']
            util.assertNear(self, expect, actual, msg=msg)
            raw_epoch = self._report.raw_epoch(host_name=host)
            msg = "Epoch count expected to be zero"
            expect = 0
            actual = raw_epoch['count']
            self.assertEqual(expect, actual, msg=msg)


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
