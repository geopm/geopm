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

"""Test that PCNT_REGION_HINTS report values match expectectations when
running the synthetic benchmark

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

if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    from integration.test import geopm_test_launcher
    geopmpy.error.exc_clear()

class TestIntegration_pcnt_region_hints(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'pcnt_region_hints'
        cls._report_path = 'test_{}.report'.format(test_name)
        cls._trace_path = 'test_{}.trace'.format(test_name)
        cls._image_path = 'test_{}.png'.format(test_name)
        cls._report_signals = "MSR::PPERF:PCNT_REGION_HINT_COMPUTE,MSR::PPERF:PCNT_REGION_HINT_MEMORY,MSR::PPERF:PCNT_REGION_HINT_IGNORE"
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_path = 'test_' + test_name + '-agent-config.json'
        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()
        if not cls._skip_launch:
            # Set the job size parameters
            cls._num_node = 1
            num_rank = 2
            time_limit = 6000
            # Configure the test application
            cls._dgemm_bigo = 50
            cls._stream_bigo = 1
            cls._loop_count = 80
            app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
            app_conf.append_region('dgemm', cls._dgemm_bigo)
            app_conf.append_region('stream', cls._stream_bigo)
            app_conf.set_loop_count(cls._loop_count)

            # Configure the monitor agent
            agent_conf = geopmpy.io.AgentConf(test_name + '_agent.config')

            # Create the test launcher with the above configuration
            launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                        agent_conf,
                                                        cls._report_path,
                                                        cls._trace_path,
                                                        time_limit=time_limit,
                                                        report_signals = cls._report_signals)
            launcher.set_num_node(cls._num_node)
            launcher.set_num_rank(num_rank)
            # Run the test application
            launcher.run('test_' + test_name)

        # Output to be reused by all tests
        cls._trace_output = geopmpy.io.AppOutput(traces=cls._trace_path + '*')
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_pcnt_region_hints._keep_files = True

    def test_load_report(self):
        """Test that the report can be loaded

        """
        report = geopmpy.io.RawReport(self._report_path)


    def test_runtime(self):
        '''Test that region and application total sync runtimes match
           PCNT REGION HINT aggregates

        '''
        for node in self._node_names:
            dgemm_data = self._report.raw_region(node, 'dgemm')
            total_runtime = (dgemm_data['MSR::PPERF:PCNT_REGION_HINT_COMPUTE'] +
                             dgemm_data['MSR::PPERF:PCNT_REGION_HINT_MEMORY'] +
                             dgemm_data['MSR::PPERF:PCNT_REGION_HINT_IGNORE'])
            util.assertNear(self, total_runtime, dgemm_data['sync-runtime (s)'])

            stream_data = self._report.raw_region(node, 'stream')

            total_runtime = (stream_data['MSR::PPERF:PCNT_REGION_HINT_COMPUTE'] +
                             stream_data['MSR::PPERF:PCNT_REGION_HINT_MEMORY'] +
                             stream_data['MSR::PPERF:PCNT_REGION_HINT_IGNORE'])
            util.assertNear(self, total_runtime, stream_data['sync-runtime (s)'])

            unmarked_data = self._report.raw_unmarked(node)
            total_runtime = (unmarked_data['MSR::PPERF:PCNT_REGION_HINT_COMPUTE'] +
                             unmarked_data['MSR::PPERF:PCNT_REGION_HINT_MEMORY'] +
                             unmarked_data['MSR::PPERF:PCNT_REGION_HINT_IGNORE'])
            util.assertNear(self, total_runtime, unmarked_data['sync-runtime (s)'])

            app_total = self._report.raw_totals(node)
            total_runtime = (app_total['MSR::PPERF:PCNT_REGION_HINT_COMPUTE'] +
                             app_total['MSR::PPERF:PCNT_REGION_HINT_MEMORY'] +
                             app_total['MSR::PPERF:PCNT_REGION_HINT_IGNORE'])
            util.assertNear(self, total_runtime, app_total['sync-runtime (s)'])


    def test_runtime_composition(self):
        '''Test that PCNT REGION HINT values for a region match expectations

        '''
        for node in self._node_names:
            # DGEMM should be at least 80% compute region
            dgemm_data = self._report.raw_region(node, 'dgemm')
            hint_runtime = dgemm_data['MSR::PPERF:PCNT_REGION_HINT_COMPUTE']
            util.assertNear(self, hint_runtime, dgemm_data['sync-runtime (s)'], 0.20)

            # DGEMM should be at least 95% compute region and memory region
            hint_runtime = (dgemm_data['MSR::PPERF:PCNT_REGION_HINT_COMPUTE'] +
                            dgemm_data['MSR::PPERF:PCNT_REGION_HINT_MEMORY'])
            util.assertNear(self, hint_runtime, dgemm_data['sync-runtime (s)'], 0.05)

            # STREAM should be at least 80% memory region and ignore region
            stream_data = self._report.raw_region(node, 'stream')
            hint_runtime = (stream_data['MSR::PPERF:PCNT_REGION_HINT_MEMORY'] +
                            stream_data['MSR::PPERF:PCNT_REGION_HINT_IGNORE'])
            util.assertNear(self, hint_runtime, stream_data['sync-runtime (s)'], 0.20)

            # app total compute hint time should be near dgemm runtime (not sync)
            app_total = self._report.raw_totals(node)
            hint_runtime = app_total['MSR::PPERF:PCNT_REGION_HINT_COMPUTE']
            util.assertNear(self, hint_runtime, dgemm_data['runtime (s)'], 0.20)

            # app total non-compute hint time should be near stream runtime (not sync)
            # when we discount any dgemm memory bound nature
            hint_runtime = (app_total['MSR::PPERF:PCNT_REGION_HINT_MEMORY'] +
                            app_total['MSR::PPERF:PCNT_REGION_HINT_IGNORE'] -
                            dgemm_data['MSR::PPERF:PCNT_REGION_HINT_MEMORY'])

            util.assertNear(self, hint_runtime, stream_data['runtime (s)'], 0.20)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
