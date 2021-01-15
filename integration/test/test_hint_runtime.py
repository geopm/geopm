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


'''Tests of different region hints, including nested regions and
unnested MPI.

'''
class TestIntegration_hint_runtime(unittest.TestCase):
    # TODO: write custom app with multiple hints, get rid of nested
    # model region from geopmbench
    def test_runtime_nested(self):
        name = 'test_runtime_nested'
        report_path = name + '.report'
        num_node = 1
        num_rank = 1
        delay = 1.0
        loop_count = 2
        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('nested-progress', delay)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path)
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        for nn in node_names:
            spin_data = self._output.get_report_data(node_name=nn, region='spin')
            epoch_data = self._output.get_report_data(node_name=nn, region='epoch')
            app_totals = self._output.get_app_total_data(node_name=nn)
            # The spin sections of this region sleep for 'delay' seconds twice per loop.
            util.assertNear(self, 2 * loop_count * delay, spin_data['runtime'].item())
            util.assertNear(self, spin_data['runtime'].item(), epoch_data['runtime'].item(), epsilon=0.01)
            self.assertGreater(app_totals['network-time'].item(), 0)
            self.assertGreater(0.1, app_totals['network-time'].item())
            self.assertEqual(loop_count, spin_data['count'].item())



    # TODO: custom app with both unnested and nested MPI and check
    # that fraction of the time is time-hint-network
    def test_network_times(self):
        name = 'test_network_times'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        for nn in node_names:
            all2all_data = self._output.get_report_data(node_name=nn, region='all2all')
            sleep_data = self._output.get_report_data(node_name=nn, region='sleep')
            dgemm_data = self._output.get_report_data(node_name=nn, region='dgemm')
            barrier_data = self._output.get_report_data(node_name=nn, region='MPI_Barrier')
            unmarked_data = self._output.get_report_data(node_name=nn, region='unmarked-region')
            epoch_data = self._output.get_report_data(node_name=nn, region='epoch')
            app_total = self._output.get_app_total_data(node_name=nn)
            self.assertEqual(0, unmarked_data['count'].item())
            # Since MPI time is is counted if any rank on a node is in
            # an MPI call, but region time is counted only when all
            # ranks on a node are in a region, we must use the
            # unmarked-region time as our error term when comparing
            # MPI time and all2all time.
            mpi_epsilon = max(unmarked_data['runtime'].item() / all2all_data['network_time'].item(), 0.05)
            util.assertNear(self, all2all_data['network_time'].item(), all2all_data['runtime'].item(), mpi_epsilon)
            util.assertNear(self, all2all_data['network_time'].item() + barrier_data['network_time'].item(),
                            epoch_data['network_time'].item())
            # TODO: inconsistent; can we just use _ everywhere?
            util.assertNear(self, all2all_data['network_time'].item() + barrier_data['network_time'].item(),
                            app_total['network-time'].item())
            self.assertEqual(0, unmarked_data['network_time'].item())
            self.assertEqual(0, sleep_data['network_time'].item())
            self.assertEqual(0, dgemm_data['network_time'].item())


    # TODO: combine with nested region test, have some unnested
    # regions with hints; call it test_hint_time
    def test_ignore_runtime(self):
        name = 'test_ignore_runtime'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('ignore', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        for nn in node_names:
            ignore_data = self._output.get_report_data(node_name=nn, region='ignore')
            startup_data = self._output.get_report_data(node_name=nn,
                                                        region='geopm_dgemm_model_region_startup')
            app_data = self._output.get_app_total_data(node_name=nn)
            util.assertNear(self, ignore_data['runtime'].item() + startup_data['runtime'].item(),
                            app_data['ignore-runtime'].item(), 0.00005)



if __name__ == '__main__':
    unittest.main()
