#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import unittest
import os
import json

import geopmpy.io
import geopmpy.agent

from integration.test import util
from integration.test import geopm_test_launcher

environment_default_path = os.path.join(util.get_config_value('GEOPM_CONFIG_PATH'), 'environment-default.json')
environment_override_path = os.path.join(util.get_config_value('GEOPM_CONFIG_PATH'), 'environment-override.json')

class TestIntegrationEnvironment(unittest.TestCase):

    def assert_geopm_uses_policy(self, expected_policy, context, user_policy=None):
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
        app_conf.append_region('sleep', 0.01)
        user_agent_conf = geopmpy.agent.AgentConf(context + '.user.agent.config', self._agent, {} if user_policy is None else user_policy)
        launcher = geopm_test_launcher.TestLauncher(app_conf, user_agent_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(context, include_geopm_policy=user_policy is not None)

        actual_policy = geopmpy.io.RawReport(report_path).meta_data()['Policy']
        for expected_key, expected_value in expected_policy.items():
            self.assertEqual(expected_value, actual_policy[expected_key],
                             msg='Wrong policy value for {} (context: {})'.format(expected_key, context))
        for actual_key, actual_value in actual_policy.items():
            if actual_key not in expected_policy:
                self.assertEqual('NAN', actual_value,
                                 msg='Unexpected value for {} (context: {})'.format(actual_key, context))

    @util.skip_unless_batch()
    @util.skip_or_ensure_writable_file(environment_default_path)
    @util.skip_or_ensure_writable_file(environment_override_path)
    @util.skip_unless_library_in_ldconfig('libgeopm.so')
    def test_geopm_environment(self):
        """Test behavior of geopm environment files.
        """
        def create_policy_file_on_compute_node(policy, policy_file_name, etc_config_path):
            policy_string = json.dumps(policy)
            policy_file_path = os.path.join('/tmp', policy_file_name)
            with open('/dev/null', 'w') as dev_null:
                util.run_script_on_compute_nodes(
                    'mv -f {policy_file} {policy_file}.backup ; '
                    'echo \'{policy}\' > {policy_file} && '
                    'echo \'{{"GEOPM_POLICY": "{policy_file}", "GEOPM_AGENT": "frequency_map"}}\' > {etc_file}'.format(
                        policy=policy_string, policy_file=policy_file_path, etc_file=etc_config_path),
                    dev_null, dev_null)

        test_name = 'test_geopm_environment'
        self._agent = 'frequency_map'
        user_policy = { 'FREQ_CPU_DEFAULT': 1.5e9 }

        with util.temporarily_remove_compute_node_file(environment_default_path), \
                util.temporarily_remove_compute_node_file(environment_override_path):
            # Only the default is set. Can be overridden by the user.
            default_policy = { 'FREQ_CPU_DEFAULT': 1.6e9 }
            create_policy_file_on_compute_node(default_policy, test_name + '.default.agent.config', environment_default_path)
            self.assert_geopm_uses_policy(default_policy, test_name + '_default_no_user')
            self.assert_geopm_uses_policy(user_policy, test_name + '_default_with_user', user_policy=user_policy)

            # Both default and override are set. Override is always used.
            override_policy = { 'FREQ_CPU_DEFAULT': 1.7e9 }
            create_policy_file_on_compute_node(override_policy, test_name + '.override.agent.config', environment_override_path)
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_and_default_no_user')
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_and_default_with_user', user_policy=user_policy)

            # Only override is set. Override is always used.
            util.remove_file_on_compute_nodes(environment_default_path)
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_no_user')
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_with_user', user_policy=user_policy)



if __name__ == '__main__':
    unittest.main()
