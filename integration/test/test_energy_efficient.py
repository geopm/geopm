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
class TestIntegration_energy_efficient(unittest.TestCase):
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
    @util.skip_unless_cpufreq()
    @util.skip_unless_batch()
    def test_agent_energy_efficient(self):
        """
        Test of the EnergyEfficientAgent.
        """
        name = 'test_energy_efficient_sticker'
        min_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
        sticker_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
        self._agent = "energy_efficient"
        num_node = 1
        num_rank = 4
        loop_count = 200
        dgemm_bigo = 15.0
        stream_bigo = 1.0
        dgemm_bigo_jlse = 35.647
        dgemm_bigo_quartz = 29.12
        stream_bigo_jlse = 1.6225
        stream_bigo_quartz = 1.7941
        hostname = socket.gethostname()
        if hostname.endswith('.alcf.anl.gov'):
            dgemm_bigo = dgemm_bigo_jlse
            stream_bigo = stream_bigo_jlse
        elif hostname.startswith('mcfly'):
            dgemm_bigo = 28.0
            stream_bigo = 1.5
        elif hostname.startswith('quartz'):
            dgemm_bigo = dgemm_bigo_quartz
            stream_bigo = stream_bigo_quartz

        run = ['_sticker', '_nan_nan']
        for rr in run:
            report_path = name + rr + '.report'
            self._tmp_files.append(report_path)
            trace_path = name + rr + '.trace'
            app_conf = geopmpy.io.BenchConf(name + '_app.config')
            self._tmp_files.append(app_conf.get_path())
            app_conf.set_loop_count(loop_count)
            app_conf.append_region('dgemm', dgemm_bigo)
            app_conf.append_region('stream', stream_bigo)
            app_conf.write()
            if rr == '_sticker':
                self._options = {'frequency_min': sticker_freq,
                                 'frequency_max': sticker_freq}
            else:
                self._options = {'frequency_min': min_freq,
                                 'frequency_max': sticker_freq}
            agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
            self._tmp_files.append(agent_conf.get_path())
            launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path,
                                                        trace_path, region_barrier=True, time_limit=900)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            launcher.run(name + rr)

        # compare the app_total runtime and energy and assert within bounds
        report_path = name + run[0] + '.report'
        trace_path = name + run[0] + '.trace'
        sticker_out = geopmpy.io.AppOutput(report_path, trace_path + '*')
        for nn in sticker_out.get_node_names():
            self._tmp_files.append(trace_path + '-{}'.format(nn))
        report_path = name + run[1] + '.report'
        trace_path = name + run[1] + '.trace'
        nan_out = geopmpy.io.AppOutput(report_path, trace_path + '*')
        for nn in nan_out.get_node_names():
            self._tmp_files.append(trace_path + '-{}'.format(nn))
            sticker_app_total = sticker_out.get_app_total_data(node_name=nn)
            nan_app_total = nan_out.get_app_total_data(node_name=nn)
            runtime_savings_epoch = (sticker_app_total['runtime'].item() - nan_app_total['runtime'].item()) / sticker_app_total['runtime'].item()
            energy_savings_epoch = (sticker_app_total['energy-package'].item() - nan_app_total['energy-package'].item()) / sticker_app_total['energy-package'].item()
            self.assertLess(-0.1, runtime_savings_epoch)  # want -10% or better
            self.assertLess(0.0, energy_savings_epoch)


    def test_agent_energy_efficient_single_region(self):
        """
        Test of the EnergyEfficientAgent against single region loop.
        """
        name = 'test_energy_efficient_single_region'
        min_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
        sticker_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
        self._agent = "energy_efficient"
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 1
        num_rank = 4
        loop_count = 100
        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('spin', 0.1)
        self._options = {'frequency_min': min_freq,
                         'frequency_max': sticker_freq}
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        regions = self._output.get_region_names()
        for nn in node_names:
            for region_name in regions:
                report = geopmpy.io.RawReport(report_path)
                if (region_name in ['spin']):
                    region = report.raw_region(nn, region_name)
                    msg = region_name + " frequency should be minimum frequency as specified by policy"
                    self.assertEqual(region['requested-online-frequency'], min_freq, msg=msg)  # freq should reduce



if __name__ == '__main__':
    unittest.main()
