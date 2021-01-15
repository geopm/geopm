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

import sys
import unittest
import os

import geopm_context
import geopmpy.io

import util


@util.skip_unless_do_launch()
class TestIntegration(unittest.TestCase):
    def setUp(self):
        self.longMessage = True
        self._agent = 'power_governor'
        self._options = {'power_budget': 150}
        self._tmp_files = []
        self._output = None
        self._power_limit = geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self._frequency = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self._original_freq_map_env = os.environ.get('GEOPM_FREQUENCY_MAP')

    def tearDown(self):
        geopm_test_launcher.geopmwrite("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0 " + str(self._power_limit))
        geopm_test_launcher.geopmwrite("MSR::PERF_CTL:FREQ board 0 " + str(self._frequency))
        if self._original_freq_map_env is None:
            if 'GEOPM_FREQUENCY_MAP' in os.environ:
                os.environ.pop('GEOPM_FREQUENCY_MAP')
        else:
            os.environ['GEOPM_FREQUENCY_MAP'] = self._original_freq_map_env

    @util.skip_unless_run_long_tests()
    def test_scaling(self):
        """
        This test will start at ${num_node} nodes and ranks.  It will then calls check_run() to
        ensure that commands can be executed successfully on all of the allocated compute nodes.
        Afterwards it will run the specified app config on each node and verify the reports.  When
        complete it will double num_node and run the steps again.

        WARNING: This test can take a long time to run depending on the number of starting nodes and
        the size of the allocation.
        """
        name = 'test_scaling'
        report_path = name + '.report'
        num_node = 2
        loop_count = 100

        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        app_conf.set_loop_count(loop_count)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path, time_limit=900)

        check_successful = True
        while check_successful:
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_node)
            try:
                launcher.check_run(name)
            except subprocess.CalledProcessError as e:
                # If we exceed the available nodes in the allocation ALPS/SLURM give a rc of 1
                # All other rc's are real errors
                if e.returncode != 1:
                    raise e
                check_successful = False
            if check_successful:
                launcher.write_log(name, 'About to run on {} nodes.'.format(num_node))
                launcher.run(name)
                self._output = geopmpy.io.AppOutput(report_path)
                node_names = self._output.get_node_names()
                self.assertEqual(len(node_names), num_node)
                for nn in node_names:
                    dgemm_data = self._output.get_report_data(node_name=nn, region='dgemm')
                    all2all_data = self._output.get_report_data(node_name=nn, region='all2all')
                    self.assertEqual(loop_count, dgemm_data['count'].item())
                    self.assertEqual(loop_count, all2all_data['count'].item())
                    self.assertGreater(dgemm_data['runtime'].item(), 0.0)
                    self.assertGreater(all2all_data['runtime'].item(), 0.0)
                num_node *= 2
                self._output.remove_files()



if __name__ == '__main__':
    unittest.main()
