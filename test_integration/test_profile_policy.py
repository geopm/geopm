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
import sys
import signal
import unittest
import subprocess

import geopmpy.launcher
import geopmpy.io
import geopmpy.policy_store
from . import util


@util.skip_unless_slurm_batch()
class TestIntegrationProfilePolicy(unittest.TestCase):
    def setUp(self):
        self.default_power_cap = 142
        self.custom_power_cap = 152
        # file name must match name in plugin
        user_home = os.environ['HOME']
        policy_db_path = "{}/policystore.db".format(user_home)
        geopmpy.policy_store.connect(policy_db_path)
        geopmpy.policy_store.set_default("power_balancer", [self.default_power_cap])
        geopmpy.policy_store.set_best("power_custom", "power_balancer", [self.custom_power_cap, 0, 0, 0])
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
        policy_path = '/geopm_endpoint_profile_policy_test'
        # test launcher sets profile, have to use real launcher for now
        self._argv = ['dummy', 'srun',
                      '--geopm-policy', policy_path,
                      '--geopm-agent', agent,
                      '--geopm-timeout', "5"]

        # launch the fake slurm plugin
        # todo: build this in the test_integration folder
        endpoint_demo_path = '/home/drguttma/geopm/examples/endpoint/.libs/geopm_policystore_demo'
        # todo: this must be launched on the node
        self._endpoint_mgr = subprocess.Popen([endpoint_demo_path,
                                               '-s', policy_path,
                                               '-p', policy_db_path],
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE)

    def tearDown(self):
        # kill policy handler in case it didn't exit on its own
        self._endpoint_mgr.send_signal(signal.SIGINT)
        self._endpoint_mgr.wait()
        sys.stdout.write(self._endpoint_mgr.stdout.read())
        sys.stdout.write(self._endpoint_mgr.stderr.read())
        #todo: check GEOPM_KEEP_FILES
        for path in self._files:
            os.unlink(path)
        # clean up shared memory
        #os.unlink("/dev/shm/geopm*")

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
        csv_data = []
        with open(policy_trace) as trace_file:
            for line in trace_file:
                if not line.startswith('#'):
                    csv_data.append(line.strip().split('|'))
        self.assertEqual(int(csv_data[-1][1]), self.default_power_cap)

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
        csv_data = []
        with open(policy_trace) as trace_file:
            for line in trace_file:
                if not line.startswith('#'):
                    csv_data.append(line.strip().split('|'))
        self.assertEqual(int(csv_data[-1][1]), self.custom_power_cap)


if __name__ == '__main__':
    unittest.main()
