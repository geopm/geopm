#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
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


import os
import sys
import unittest
import shlex
import subprocess
import io
import json

import geopmpy.io
import geopmdpy.topo
import geopmdpy.pio

from integration.test import geopm_test_launcher
from integration.test import util


def getSystemConfig():
    settings = {}
    for option in ["--config-default", "--config-override"]:
        try:
            proc = subprocess.Popen(shlex.split("geopmadmin {}".format(option)),
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            proc.wait()
            with proc.stdout:
                config_file = proc.stdout.readline().decode()
            with open(config_file.strip(), "r") as infile:
                settings.update(json.load(infile))
        except IOError:
            # config_file may not exist and will cause Popen to fail
            pass
    return settings


def getSystemConfigAgent():
    ret = ''
    try:
        ret = getSystemConfig()['GEOPM_AGENT']
    except LookupError:
        pass
    return ret


def getSystemConfigPolicy():
    geopm_system_config = getSystemConfig()
    policy = {}
    with open(geopm_system_config['GEOPM_POLICY'], 'r') as infile:
        policy = json.load(infile)
    return policy


def skip_if_geopmadmin_check_fails():
    try:
        subprocess.check_call(shlex.split("geopmadmin"),
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except:
        return unittest.skip("geopmadmin check failed, there is an issue with the site configuration.")
    return lambda func: func


def skip_unless_energy_agent():
    agent = ''
    try:
        agent = getSystemConfigAgent()
    except BaseException as ex:
        return unittest.skip('geopmadmin check failed: {}'.format(ex))
    if agent not in ['frequency_map', 'energy_efficient']:
        return unittest.skip('Requires environment default/override to be configured to cap frequency.')
    return lambda func: func


def skip_unless_power_agent():
    agent = ''
    try:
        agent = getSystemConfigAgent()
    except BaseException as ex:
        return unittest.skip('geopmadmin check failed: {}'.format(ex))
    if agent not in ['power_governor', 'power_balancer']:
        return unittest.skip('Requires environment default/override to be configured to cap power.')
    return lambda func: func


@util.skip_unless_batch()
@skip_if_geopmadmin_check_fails()
class TestIntegrationPluginStaticPolicy(unittest.TestCase):
    """Test the static policy enforcement feature of the currently
       configured RM plugin.

    """
    @classmethod
    def setUpClass(cls):
        cls._tmp_files = []
        cls._geopmadminagent = getSystemConfigAgent()
        cls._geopmadminagentpolicy = getSystemConfigPolicy()

    @skip_unless_energy_agent()
    def test_frequency_cap_enforced(self):
        policy_name = None
        if self._geopmadminagent == 'frequency_map':
            policy_name = 'FREQ_MAX'
        elif self._geopmadminagent == 'energy_efficient':
            policy_name = 'FREQ_FIXED'
        try:
            test_freq = self._geopmadminagentpolicy[policy_name]
            current_freq = geopmdpy.pio.read_signal("MSR::PERF_CTL:FREQ", "board", 0)
            self.assertEqual(test_freq, current_freq)
        except KeyError:
            self.skipTest('Expected frequency cap "{}" for agent missing from policy'.format(policy_name))

    @skip_unless_power_agent()
    def test_power_cap_enforced(self):
        num_pkg = geopmdpy.topo.num_domain('package')
        policy_name = 'POWER_PACKAGE_LIMIT_TOTAL'
        try:
            test_power = self._geopmadminagentpolicy[policy_name] / num_pkg
            for pkg in range(num_pkg):
                current_power = geopmdpy.pio.read_signal("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", "package", pkg)
                self.assertEqual(test_power, current_power)
        except KeyError:
            self.skipTest('Expected power cap "{}" for agent missing from policy'.format(policy_name))


if __name__ == '__main__':
    unittest.main()
