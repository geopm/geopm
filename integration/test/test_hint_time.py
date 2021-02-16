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

"""HINT_TIME

"""

import sys
import unittest
import os
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from integration.test import geopm_context
import geopmpy.io
import geopmpy.error

from integration.test import util
from integration.experiment import machine
if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    from integration.test import geopm_test_launcher
    geopmpy.error.exc_clear()

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
        bin_name = 'test_hint_time'
        # Look for in place build
        script_dir = os.path.dirname(os.path.realpath(__file__))
        bin_path = os.path.join(script_dir, '.libs', bin_name)
        if not os.path.exists(bin_path):
            # Look for out of place build from using apps/build_func.sh
            int_dir = os.path.dirname(script_dir)
            bin_path_op = os.path.join(int_dir, 'build/integration/test/.libs', bin_name)
            if not os.path.exists(bin_path_op):
                msg = 'Could not find application binary, tried \n    "{}"\n    "{}"'.format(
                      bin_path, bin_path_op)
                raise RuntimeError(msg)
            bin_path = bin_path_op
        return bin_path

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


class TestIntegration_hint_time(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'hint_time'
        cls._report_path = 'test_{}.report'.format(cls._test_name)
        cls._image_path = 'test_{}.png'.format(cls._test_name)
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_path = 'test_' + cls._test_name + '-agent-config.json'
        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()
        machine_file_name = 'test_{}.machine'.format(cls._test_name)
        cls._machine = machine.Machine()
        try:
            cls._machine.load()
        except RuntimeError:
            cls._machine.save()

        if not cls._skip_launch:
            # Set the job size parameters
            cls._num_node = 1
            num_rank = 1
            time_limit = 60
            # Configure the test application
            app_conf = AppConf()
            agent_conf = geopmpy.io.AgentConf(cls._agent_conf_path)

            # Create the test launcher with the above configuration
            launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                        agent_conf,
                                                        cls._report_path,
                                                        time_limit=time_limit)
            launcher.set_num_node(cls._num_node)
            launcher.set_num_rank(num_rank)
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
            for region_name in self._report.region_names(host):
                raw_region = self._report.raw_region(host_name=host,
                                                     region_name=region_name)
                if region_name == 'network':
                    found_nw = True
                    msg = "{}: Should be one second of network time".format(region_name)
                    expect = 1.0
                    actual = raw_region['time-hint-network (s)']
                    util.assertNear(self, expect, actual, msg=msg)
                    msg = "{}: Should be one second of total time".format(region_name)
                    actual = raw_region['sync-runtime (s)']
                    util.assertNear(self, expect, actual, msg=msg)
                if region_name == 'network-memory':
                    found_nw_mem = True
                    msg = "{}: Should have two seconds of network time".format(region_name)
                    expect = 2.0
                    actual = raw_region['time-hint-network (s)']
                    util.assertNear(self, expect, actual, msg=msg)
                    msg = "{}: Should have one second of memory time".format(region_name)
                    expect = 1.0
                    actual = raw_region['time-hint-memory (s)']
                    util.assertNear(self, expect, actual, msg=msg)
                    msg = "{}: Should be three seconds of total time".format(region_name)
                    expect = 3.0
                    actual = raw_region['sync-runtime (s)']
                    util.assertNear(self, expect, actual, msg=msg)
            self.assertTrue(found_nw)
            self.assertTrue(found_nw_mem)
            raw_totals = self._report.raw_totals(host_name=host)
            msg = "Application totals should have three seconds of network time"
            expect = 3.0
            actual = raw_totals['time-hint-network (s)']
            util.assertNear(self, expect, actual, msg=msg)
            msg = "Application totals should have one second of memory time"
            expect = 1.0
            actual = raw_totals['time-hint-memory (s)']
            util.assertNear(self, expect, actual, msg=msg)
            msg = "Application totals should have four seconds of total time"
            expect = 4.0
            actual = raw_totals['sync-runtime (s)']
            util.assertNear(self, expect, actual, msg=msg)

            raw_epoch = self._report.raw_epoch(host_name=host)
            msg = "Epoch should have two seconds of network time"
            expect = 1.0
            actual = raw_epoch['time-hint-network (s)']
            util.assertNear(self, expect, actual, msg=msg)
            msg = "Epoch should have one second of memory time".format(region_name)
            expect = 1.0
            actual = raw_epoch['time-hint-memory (s)']
            util.assertNear(self, expect, actual, msg=msg)
            msg = "Epoch should have three seconds of total time".format(region_name)
            expect = 2.0
            actual = raw_epoch['sync-runtime (s)']
            util.assertNear(self, expect, actual, msg=msg)


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
