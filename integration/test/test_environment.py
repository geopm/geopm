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
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 0.01)
        user_agent_conf = geopmpy.io.AgentConf(context + '.user.agent.config', self._agent, {} if user_policy is None else user_policy)
        self._tmp_files.append(user_agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, user_agent_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(context, include_geopm_policy=user_policy is not None)
        self._tmp_files.append(report_path)

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
    @util.skip_unless_library_in_ldconfig('libgeopmpolicy.so')
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
                    'echo \'{{"GEOPM_POLICY": "{policy_file}", "GEOPM_AGENT": "energy_efficient"}}\' > {etc_file}'.format(
                        policy=policy_string, policy_file=policy_file_path, etc_file=etc_config_path),
                    dev_null, dev_null)

        test_name = 'test_geopm_environment'
        self._agent = 'energy_efficient'
        user_policy = { 'PERF_MARGIN': 0.13 }

        with util.temporarily_remove_compute_node_file(environment_default_path), \
                util.temporarily_remove_compute_node_file(environment_override_path):
            # Only the default is set. Can be overridden by the user.
            default_policy = { 'PERF_MARGIN': 0.11 }
            create_policy_file_on_compute_node(default_policy, test_name + '.default.agent.config', environment_default_path)
            self.assert_geopm_uses_policy(default_policy, test_name + '_default_no_user')
            self.assert_geopm_uses_policy(user_policy, test_name + '_default_with_user', user_policy=user_policy)

            # Both default and override are set. Override is always used.
            override_policy = { 'PERF_MARGIN': 0.12 }
            create_policy_file_on_compute_node(override_policy, test_name + '.override.agent.config', environment_override_path)
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_and_default_no_user')
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_and_default_with_user', user_policy=user_policy)

            # Only override is set. Override is always used.
            util.remove_file_on_compute_nodes(environment_default_path)
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_no_user')
            self.assert_geopm_uses_policy(override_policy, test_name + '_override_with_user', user_policy=user_policy)



if __name__ == '__main__':
    unittest.main()
