#!/usr/bin/env python

import os
import unittest
import geopmpy.launcher
import geopmpy.io
import geopmpy.policy_store


class TestIntegrationProfilePolicy(unittest.TestCase):
    def setUp(self):
        # file name must match name in plugin
        user_home = os.environ['HOME']
        policy_db_path = "{}/policystore.db".format(user_home)
        geopmpy.policy_store.connect(policy_db_path)
        geopmpy.policy_store.set_default("power_balancer", [152])
        geopmpy.policy_store.set_best("power_142", "power_balancer", [142, 0, 0, 0])
        geopmpy.policy_store.disconnect()

        # common run parameters
        self._num_rank = 1
        self._num_node = 1
        agent = 'power_balancer'
        self._app_conf = geopmpy.io.BenchConf('test_profile_policy_app.config')
        self._app_conf.append_region('sleep', 1.0)
        self._app_conf.write()
        self._files = []
        self._files.append(self._app_conf.get_path())
        # test launcher sets profile, have to use real launcher for now
        self._argv = ['dummy', 'srun',
                      '--geopm-agent', agent,
                      '--reservation', 'diana']  # todo: temporary

    def tearDown(self):
        for path in self._files:
            os.unlink(path)

    def test_policy_default(self):
        profile = 'unknown'
        report_path = profile + '.report'
        self._files.append(report_path)
        self._argv.extend(['--geopm-profile', profile])
        self._argv.extend(['--geopm-report', report_path])
        self._argv.append(self._app_conf.get_exec_path())
        self._argv.extend(self._app_conf.get_exec_args())
        launcher = geopmpy.launcher.Factory().create(self._argv, self._num_rank, self._num_node)
        launcher.run()

        report_data = geopmpy.io.RawReport(report_path).meta_data()
        self.assertEqual(report_data['Profile'], profile)
        policy = report_data['Policy']
        self.assertEqual(policy['POWER_CAP'], 152)

    def test_policy_142(self):
        profile = 'power_142'
        report_path = profile + '.report'
        self._argv.extend(['--geopm-profile', profile])
        self._argv.extend(['--geopm-report', report_path])
        self._argv.append(self._app_conf.get_exec_path())
        self._argv.extend(self._app_conf.get_exec_args())
        launcher = geopmpy.launcher.Factory().create(self._argv, self._num_rank, self._num_node)
        launcher.run()

        report_data = geopmpy.io.RawReport(report_path).meta_data()
        self.assertEqual(report_data['Profile'], profile)
        policy = report_data['Policy']
        self.assertEqual(policy['POWER_CAP'], 142)

if __name__ == '__main__':
    unittest.main()
