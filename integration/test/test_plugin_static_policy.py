#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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


def skip_unless_freq_agent():
    agent = ''
    try:
        agent = getSystemConfigAgent()
    except BaseException as ex:
        return unittest.skip('geopmadmin check failed: {}'.format(ex))
    if agent not in ['frequency_map']:
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

    @skip_unless_freq_agent()
    def test_frequency_cap_enforced(self):
        policy_name = None
        if self._geopmadminagent == 'frequency_map':
            policy_name = 'FREQ_MAX'
        try:
            test_freq = self._geopmadminagentpolicy[policy_name]
            current_freq = geopmdpy.pio.read_signal("MSR::PERF_CTL:FREQ", "board", 0)
            self.assertEqual(test_freq, current_freq)
        except KeyError:
            self.skipTest('Expected frequency cap "{}" for agent missing from policy'.format(policy_name))

    @skip_unless_power_agent()
    def test_power_cap_enforced(self):
        num_pkg = geopmdpy.topo.num_domain('package')
        policy_name = 'CPU_POWER_LIMIT'
        try:
            test_power = self._geopmadminagentpolicy[policy_name] / num_pkg
            for pkg in range(num_pkg):
                current_power = geopmdpy.pio.read_signal("MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT", "package", pkg)
                self.assertEqual(test_power, current_power)
        except KeyError:
            self.skipTest('Expected power cap "{}" for agent missing from policy'.format(policy_name))

    # TODO: move shared code to util.py
    def assert_geopm_uses_agent_policy(self, expected_agent, expected_policy, context):
        """Assert that geopm uses the given policy.
        Arguments:
        expected_policy (dict str->float): Policy to expect in generated reports.
        context (str): Additional context for test file names and failure messages.
        user_policy (dict str->float): Policy to request in the geopmlaunch command.
        """
        report_path = '{}.report'.format(context)
        num_node = 1
        num_rank = 1
        app_conf = geopmpy.io.BenchConf(context + '.app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 0.01)
        launcher = geopm_test_launcher.TestLauncher(app_conf=app_conf, report_path=report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(context)
        self._tmp_files.append(report_path)

        actual_agent = geopmpy.io.RawReport(report_path).meta_data()['Agent']
        self.assertEqual(expected_agent, actual_agent)
        actual_policy = geopmpy.io.RawReport(report_path).meta_data()['Policy']
        for expected_key, expected_value in expected_policy.items():
            self.assertEqual(expected_value, actual_policy[expected_key],
                             msg='Wrong policy value for {} (context: {})'.format(expected_key, context))
        for actual_key, actual_value in actual_policy.items():
            if actual_key not in expected_policy:
                self.assertEqual('NAN', actual_value,
                                 msg='Unexpected value for {} (context: {})'.format(actual_key, context))

    @unittest.skipUnless(getSystemConfigAgent() != '',
                         'Requires environment default/override files to be configured with an agent')
    def test_agent_enforced(self):
        test_name = 'test_agent_enforced'
        config = getSystemConfig()
        agent = config['GEOPM_AGENT']
        with open(config['GEOPM_POLICY'], 'r') as pol_file:
            policy = json.load(pol_file)
        self.assert_geopm_uses_agent_policy(expected_agent=agent, expected_policy=policy, context=test_name)


if __name__ == '__main__':
    unittest.main()
