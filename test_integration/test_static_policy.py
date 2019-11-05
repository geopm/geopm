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

from __future__ import absolute_import

import os
import io
import sys
import unittest
import subprocess

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_context
import geopmpy.io
from test_integration import geopm_test_launcher
from test_integration import util


@util.skip_unless_slurm_batch()
class TestIntegrationStaticPolicy(unittest.TestCase):
    """Test the static policy enforcement feature of the agent interface

    """

    @classmethod
    def setUpClass(cls):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._app_exec_path = os.path.join(script_dir, '.libs', 'test_static_policy')

    def setUp(self):
        # make sure controls are at default
        max_freq = geopm_test_launcher.geopmread("FREQUENCY_MAX board 0")
        geopm_test_launcher.geopmwrite("FREQUENCY board 0 {}".format(max_freq))
        tdp_power = geopm_test_launcher.geopmread("POWER_PACKAGE_TDP package 0")
        geopm_test_launcher.geopmwrite("POWER_PACKAGE_LIMIT board 0 {}".format(tdp_power))

        self._stdout = None  # io.StringIO()
        self._stderr = None  # io.StringIO()

        self._old_max_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self._old_max_power = geopm_test_launcher.geopmread("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0")

    def tearDown(self):
        if self._stdout:
            sys.stdout.write('{}\n'.format(self._stdout))  # self._stdout.getvalue()))
        if self._stderr:
            sys.stderr.write('{}\n'.format(self._stderr))  # getvalue()))

        geopm_test_launcher.geopmwrite("MSR::PERF_CTL:FREQ board 0 {}".format(self._old_max_freq))
        geopm_test_launcher.geopmwrite("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT board 0 {}".format(self._old_max_power))

    def run_tool(self, agent_name, policy_setting):
        test_name = 'test_static_policy'
        self._report_path = test_name + '.report'
        self._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None
        self._agent_conf_path = test_name + '-agent-config.json'
        agent_conf = geopmpy.io.AgentConf(self._agent_conf_path,
                                          agent_name,
                                          policy_setting)
        agent_conf.write()

        # set environment in launch using geopmlauncher options
        environ = {
            "GEOPM_POLICY": agent_conf.get_path(),
            "GEOPM_AGENT": agent_name
        }
        # argv = "dummy -- {app}".format(app=self._app_exec_path)
        # launch the fake slurm plugin
        # geopm_test_launcher.allocation_node_test(argv, self._stdout, self._stderr, environ)

        # TODO: detect correct launcher and set options

        prog = subprocess.Popen(["srun", "-N", "1", "-n", "1", self._app_exec_path],
                                env=environ,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        self._stdout, self._stderr = prog.communicate()

    def test_monitor_no_policy(self):
        pass

    def test_freq_map_max_freq(self):
        # changes the perf_ctl control register
        pass

    def test_energy_efficient_max_freq(self):
        sticker_freq = geopm_test_launcher.geopmread("FREQUENCY_STICKER board 0")
        step_freq = geopm_test_launcher.geopmread("FREQUENCY_STEP board 0")
        test_freq = sticker_freq - 2 * step_freq
        current_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self.assertNotEqual(test_freq, current_freq)
        self.run_tool('energy_efficient', {'frequency_max': test_freq})

        current_freq = geopm_test_launcher.geopmread("MSR::PERF_CTL:FREQ board 0")
        self.assertEqual(test_freq, current_freq)

    def test_power_governor_power_cap(self):
        pass

    def test_power_balancer_power_cap(self):
        pass


if __name__ == '__main__':
    unittest.main()
