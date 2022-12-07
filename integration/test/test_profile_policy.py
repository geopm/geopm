#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import os
import sys
import pandas
import signal
import unittest
import subprocess

import geopmpy.launcher
import geopmpy.io
import geopmpy.policy_store

from integration.test import util


@util.skip_unless_do_launch()
@util.skip_unless_batch()
class TestIntegrationProfilePolicy(unittest.TestCase):
    def setUp(self):
        # clean up stale keys
        try:
            os.unlink("/dev/shm/geopm*")
        except:
            pass

        self._files = []
        self.default_power_cap = 142
        self.custom_power_cap = 152
        # file name must match name in .cpp file
        script_dir = os.path.dirname(os.path.realpath(__file__))
        exe_path = os.path.join(script_dir, '.libs', 'test_profile_policy')
        policy_db_path = os.path.join(os.getcwd(), 'policystore.db')
        self._files.append(policy_db_path)
        geopmpy.policy_store.connect(policy_db_path)
        geopmpy.policy_store.set_default("power_balancer",
                                         [self.default_power_cap])
        geopmpy.policy_store.set_best("power_balancer", "power_custom",
                                      [self.custom_power_cap, 0, 0, 0])
        geopmpy.policy_store.disconnect()

        # common run parameters
        self._num_rank = 1
        self._num_node = 1
        agent = 'power_balancer'
        self._app_conf = geopmpy.io.BenchConf('test_profile_policy_app.config')
        self._app_conf.append_region('sleep', 1.0)
        self._app_conf.write()
        self._files.append(self._app_conf.get_path())
        # must match prefix in .cpp file
        endpoint_prefix = '/geopm_endpoint_profile_policy_test'
        # test launcher sets profile, have to use real launcher for now
        self._argv = ['dummy', 'srun',
                      '--geopm-endpoint', endpoint_prefix,
                      '--geopm-agent', agent,
                      '--geopm-timeout', "5"]

        # this must be launched on the same node as the root controller
        # for now limit test to one node
        self._endpoint_mgr = subprocess.Popen([exe_path],
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE)

    def tearDown(self):
        # kill policy handler in case it didn't exit on its own
        self._endpoint_mgr.kill()
        sys.stdout.write(self._endpoint_mgr.stdout.read().decode())
        sys.stdout.write(self._endpoint_mgr.stderr.read().decode())

        # clean up shared memory
        try:
            os.unlink("/dev/shm/geopm*")
        except:
            pass

    @util.skip_unless_config_enable('beta')
    def test_policy_default(self):
        profile = 'unknown'
        report_path = profile + '.report'
        policy_trace = profile + '.trace-policy'
        self._files.append(report_path)
        self._files.append(policy_trace)
        self._argv.extend(['--geopm-profile', profile])
        self._argv.extend(['--geopm-trace-endpoint-policy', policy_trace])
        self._argv.extend(['--geopm-report', report_path])
        self._argv.append(self._app_conf.get_exec_path())
        self._argv.extend(self._app_conf.get_exec_args())
        launcher = geopmpy.launcher.Factory().create(self._argv, self._num_rank, self._num_node)
        launcher.run()

        report_data = geopmpy.io.RawReport(report_path).meta_data()
        self.assertEqual(report_data['Profile'], profile)
        policy = report_data['Policy']
        self.assertEqual(policy, 'DYNAMIC')
        # check profile trace for single line with this power cap
        csv_data = pandas.read_csv(policy_trace, delimiter='|', comment='#')
        self.assertEqual(csv_data['CPU_POWER_LIMIT'][0], self.default_power_cap)

    @util.skip_unless_config_enable('beta')
    def test_policy_custom(self):
        profile = 'power_custom'
        report_path = profile + '.report'
        policy_trace = profile + '.trace-policy'
        self._files.append(report_path)
        self._files.append(policy_trace)
        self._argv.extend(['--geopm-profile', profile])
        self._argv.extend(['--geopm-trace-endpoint-policy', policy_trace])
        self._argv.extend(['--geopm-report', report_path])
        self._argv.append(self._app_conf.get_exec_path())
        self._argv.extend(self._app_conf.get_exec_args())
        launcher = geopmpy.launcher.Factory().create(self._argv, self._num_rank, self._num_node)
        launcher.run()

        report_data = geopmpy.io.RawReport(report_path).meta_data()
        self.assertEqual(report_data['Profile'], profile)
        policy = report_data['Policy']
        self.assertEqual(policy, 'DYNAMIC')
        # check profile trace for single line with this power cap
        csv_data = pandas.read_csv(policy_trace, delimiter='|', comment='#')
        self.assertEqual(csv_data['CPU_POWER_LIMIT'][0], self.custom_power_cap)


if __name__ == '__main__':
    unittest.main()
