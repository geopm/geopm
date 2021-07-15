#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

import sys
import unittest
import os

import geopm_context
import geopmpy.io
import geopmpy.error
import geopm_test_launcher


class TestIntegration_launch_pthread(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_launch_pthread'
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
        launcher.set_pmpi_ctl('pthread')
        launcher.run(cls._test_name)

    def test_report_and_trace_generation_pthread(self):
        '''Test that GEOPM can be launched with the controller in a spawned
           pthread.

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
