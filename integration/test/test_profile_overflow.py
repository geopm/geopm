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

"""
Runs an application with a large number of short regions and checks
that the controller successfully runs.
"""

import sys
import unittest
import os
import subprocess
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from integration.test import geopm_context
import geopmpy.io
import geopmpy.error
import geopmpy.topo
import geopm_test_launcher
import check_trace


class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the profile_overflow benchmark.

    """
    def write(self):
        """Called by the test launcher prior to executing the test application
        to write any files required by the application.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark filled in by template automatically.

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_profile_overflow')

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


class TestIntegration_profile_overflow(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_profile_overflow'
        cls._report_path = '{}.report'.format(test_name)
        cls._trace_path = '{}.trace'.format(test_name)
        cls._log_path = '{}.log'.format(test_name)
        cls._agent_conf_path = test_name + '-agent-config.json'
        # Set the job size parameters such that we have a 3 level tree
        os.environ["GEOPM_MAX_FAN_OUT"] = "2"
        num_node = 4
        num_rank = geopmpy.topo.num_domain(geopmpy.topo.DOMAIN_CORE) - 2
        time_limit = 600
        # Configure the test application
        app_conf = AppConf()
        agent_conf = geopmpy.io.AgentConf(cls._agent_conf_path)

        # Configure the agent
        # Query for the min and sticker frequency and run the
        # energy efficient agent over this range.
        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path,
                                                    cls._trace_path,
                                                    time_limit=time_limit)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        try:
            launcher.run(test_name)
        except subprocess.CalledProcessError:
            sys.stderr.write('{} failed; check log for details.\n'.format(test_name))
            raise

    @classmethod
    def tearDownClass(cls):
        os.environ.pop("GEOPM_MAX_FAN_OUT")

    def test_load_report(self):
        '''
        Test that the report can be loaded.
        '''
        report = geopmpy.io.RawReport(self._report_path)
        hosts = report.host_names()
        for hh in hosts:
            runtime = report.raw_totals(hh)['runtime (s)']
            self.assertNotEqual(0, runtime)

    def test_short_region_count(self):
        '''
        Test that the count for MPI_Barrier is as expected.
        '''
        report = geopmpy.io.RawReport(self._report_path)
        hosts = report.host_names()
        for hh in hosts:
            region_data = report.raw_region(hh, 'MPI_Barrier')
            count = region_data['count']
            self.assertEqual(count, 10000000)

    def test_sample_rate(self):
        '''
        Test that the sample rate is regular.
        '''
        traces = glob.glob(self._trace_path + "*")
        if len(traces) == 0:
            raise RuntimeError("No traces found with prefix: {}".format(self._trace_path_prefix))
        for tt in traces:
            check_trace.check_sample_rate(tt, 0.005)


if __name__ == '__main__':
    unittest.main()
