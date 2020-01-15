#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

"""Test the detection of user defined regions in an application
   configured such that the outer loop is an OMP paralell region.

"""

from __future__ import absolute_import

import sys
import re
import unittest
import os
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_context
import geopmpy.io
from test_integration import geopm_test_launcher
from test_integration import util

_g_skip_launch = False


class AppConf(object):
    """Class that is used by the test launcher as a geopmpy.io.BenchConf
    when running the script as a benchmark.

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
        cls._expected_regions = ['spin', 'all2all']
        cls._report_path = []
        cls._trace_path = []
        cls._skip_launch = _g_skip_launch
        cls._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None
        num_node = 1
        num_rank = 4
        for config in test_config:
            curr_run = test_name + config
            report_path = curr_run + '.report'
            trace_path = curr_run + '.trace'
            cls._report_path.append(report_path)
            cls._trace_path.append(trace_path)
            cls._agent_conf_path = test_name + '-agent-config.json'
            agent_conf = geopmpy.io.AgentConf(cls._agent_conf_path)
            launcher = geopm_test_launcher.TestLauncher(AppConf(),
                                                        agent_conf,
                                                        report_path,
                                                        trace_path,
                                                        time_limit=6000)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            if config == '_without_ompt':
                launcher.disable_ompt()
            if  not cls._skip_launch:
                launcher.run(curr_run)

    @classmethod
    def tearDownClass(cls):
        """If we are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset, clean up output.

        """
        if (sys.exc_info() == (None, None, None) and not
            cls._keep_files and not cls._skip_launch):
            os.unlink(cls._report_path)
            for ff in glob.glob(cls._trace_path + '*'):
                os.unlink(ff)

    def test_user_defined_regions_absent(self):
        """Test that the first run's report does NOT contain the spin and
           stream regions.
        """
        # self._report_path[0] maps to with-ompt config
        report = geopmpy.io.RawReport(self._report_path[0])
        host_names = report.host_names()
        for host_name in report.host_names():
            for expected_region in self._expected_regions:
                self.assertFalse(expected_region in report.region_names(host_name),
                                 msg='Region {} should not be present in report'.format(expected_region))

    def test_user_defined_regions_present(self):
        """Test that the second run's report DOES contain the spin and
           stream regions.
        """
        # self._report_path[1] maps to with-ompt config
        report = geopmpy.io.RawReport(self._report_path[1])
        host_names = report.host_names()
        for host_name in report.host_names():
            for expected_region in self._expected_regions:
                self.assertTrue(expected_region in report.region_names(host_name),
                                 msg='Region {} should be present in report'.format(expected_region))
                observed_region = report.raw_region(host_name, expected_region)
                expected_count = 50
                self.assertEqual(expected_count, observed_region['count'],
                                 msg='Region {} should have count {} in report'.format(expected_region, expected_count))


if __name__ == '__main__':
    try:
        sys.argv.remove('--skip-launch')
        _g_skip_launch = True
    except ValueError:
        pass
    unittest.main()
