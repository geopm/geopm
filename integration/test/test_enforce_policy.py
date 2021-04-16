#!/usr/bin/env python
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

from __future__ import absolute_import

import os
import sys
import unittest
import subprocess
import io
import json

import geopm_context
import geopmpy.agent
import geopm_test_launcher
import util
import geopmpy.topo


@util.skip_unless_do_launch()
@util.skip_unless_batch()
class TestIntegrationEnforcePolicy(unittest.TestCase):
    """Test the static policy enforcement feature of the agent interface

    """
    @classmethod
    def setUpClass(cls):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._app_exec_path = os.path.join(script_dir, '.libs', 'test_enforce_policy')

        # note: if /etc/geopm/environment-*.json sets the same variables, this
        # test will not work.
        for env_file in ['/etc/geopm/environment-default.json',
                         '/etc/geopm/environment-override.json']:
            try:
                stdout = io.StringIO()
                stderr = io.StringIO()
                geopm_test_launcher.allocation_node_test('dummy -- cat {}'.format(env_file),
                                                         stdout, stderr)
                file_contents = stdout.getvalue()
                if "GEOPM_POLICY" in file_contents:
                    raise RuntimeError("{} contains GEOPM_POLICY".format(env_file))
                if "GEOPM_AGENT" in file_contents:
                    raise RuntimeError("{} contains GEOPM_AGENT".format(env_file))
            except subprocess.CalledProcessError:
                pass

    def setUp(self):
        self._old_max_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self._old_max_power = geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")

        # make sure controls are at default
        self._max_freq = geopm_test_launcher.geopmread("FREQUENCY_MAX board 0")
        geopm_test_launcher.geopmwrite("FREQUENCY board 0 {}".format(self._max_freq))
        self._tdp_power = geopm_test_launcher.geopmread("POWER_PACKAGE_TDP package 0")
        geopm_test_launcher.geopmwrite("POWER_PACKAGE_LIMIT board 0 {}".format(self._tdp_power))
        self._sticker_freq = geopm_test_launcher.geopmread("FREQUENCY_STICKER board 0")
        self._step_freq = geopm_test_launcher.geopmread("FREQUENCY_STEP board 0")

        self._stdout = None
        self._stderr = None

    def tearDown(self):
        geopm_test_launcher.geopmwrite("MSR::PERF_CTL:FREQ board 0 {}".format(self._old_max_freq))
        geopm_test_launcher.geopmwrite("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0 {}".format(self._old_max_power))

    def run_tool(self, agent_name, policy_setting):
        test_name = 'test_enforce_policy'
        self._report_path = test_name + '.report'
        self._agent_conf_path = test_name + '-agent-config.json'

        agent_conf = geopmpy.agent.AgentConf(self._agent_conf_path,
                                             agent_name,
                                             policy_setting)
        agent_conf.write()

        # set environment in launch using geopmlaunch options instead of
        # files from /etc/geopm
        environ = os.environ.copy()
        environ.update({
            "GEOPM_POLICY": agent_conf.get_path(),
            "GEOPM_AGENT": agent_name
        })

        # detect correct launcher type and set options
        # but run without geopmlaunch
        launcher_type = geopm_test_launcher.detect_launcher()
        argv = ["dummy", launcher_type, "--geopm-ctl-disable"]
        launcher = geopmpy.launcher.Factory().create(argv, num_rank=1, num_node=1)
        num_rank = launcher.num_rank_option(False)
        num_node = launcher.num_node_option()
        argv = [launcher_type] + num_rank + num_node + [self._app_exec_path]
        prog = subprocess.Popen(argv,
                                env=environ,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        try:
            self._stdout, self._stderr = prog.communicate(timeout=10)
        except TypeError:
            self._stdout, self._stderr = prog.communicate()

    def test_monitor_no_policy(self):
        # check that the monitor doesn't change anything
        start_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        start_power = geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self.run_tool('monitor', {})
        end_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        end_power = geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self.assertEqual(start_freq, end_freq)
        self.assertEqual(start_power, end_power)

    def test_freq_map_max_freq(self):
        test_freq = self._sticker_freq - 2 * self._step_freq
        current_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self.assertNotEqual(test_freq, current_freq)
        self.run_tool('frequency_map', {'FREQ_DEFAULT': test_freq})
        current_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self.assertEqual(test_freq, current_freq)

    @unittest.skip('Disabled pending overhaul of agent.')
    def test_energy_efficient_max_freq(self):
        test_freq = self._sticker_freq - 2 * self._step_freq
        current_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self.assertNotEqual(test_freq, current_freq)
        self.run_tool('energy_efficient', {'FREQ_FIXED': test_freq})
        current_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self.assertEqual(test_freq, current_freq)

    def test_power_governor_power_cap(self):
        num_pkg = geopmpy.topo.num_domain('package')
        test_power = self._tdp_power * num_pkg - 20
        current_power = num_pkg * geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self.assertNotEqual(test_power, current_power)
        self.run_tool('power_governor', {'power_budget': test_power})
        current_power = num_pkg * geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self.assertEqual(test_power, current_power)

    def test_power_balancer_power_cap(self):
        num_pkg = geopmpy.topo.num_domain('package')
        test_power = self._tdp_power * num_pkg - 20
        current_power = num_pkg * geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self.assertNotEqual(test_power, current_power)
        self.run_tool('power_balancer', {'power_budget': test_power})
        current_power = num_pkg * geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")
        self.assertEqual(test_power, current_power)


if __name__ == '__main__':
    unittest.main()
