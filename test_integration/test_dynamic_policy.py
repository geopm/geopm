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
import unittest
import subprocess
import io
import json
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_context
import geopmpy.io
from test_integration import geopm_test_launcher
from test_integration import util
import geopmpy.topo


_g_skip_launch = False

@util.skip_unless_batch()
class TestIntegrationDynamicPolicy(unittest.TestCase):
    """Test the static policy enforcement feature of the agent interface

    """
    @classmethod
    def setUpClass(cls):
        test_name = 'test_dynamic_policy'
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._endpoint_policy_trace = test_name + '.policy'
        cls._endpoint_path = '/geopm_test_dynamic_policy'  # must match path in cpp file
        cls._sample_log = 'test_dynamic_policy_sample.log'  # must match path in cpp file
        cls._file_list = []
        cls._file_list.append(cls._report_path)
        cls._file_list.append(cls._sample_log)
        script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._daemon_exe = os.path.join(script_dir, '.libs', 'test_dynamic_policy')

        # TODO: forgot why this was needed
        cls._skip_launch = _g_skip_launch
        cls._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None

        sys.stdout.write("setUpClass\n")

        if cls._skip_launch:
            return

        num_node = 1
        num_rank = 4
        timeout = 60

        # can't use test launcher because we want to use endpoint instead of policy file

        # launch daemon in background
        launcher_type = geopm_test_launcher.detect_launcher()
        argv = ["dummy", launcher_type, "--geopm-ctl-disable"]
        launcher = geopmpy.launcher.Factory().create(argv, num_rank=1, num_node=1)
        num_rank_opt = launcher.num_rank_option(False)
        num_node_opt = launcher.num_node_option()
        argv = [launcher_type] + num_rank_opt + num_node_opt + [cls._daemon_exe]
        daemon = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # launch app
        app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
        cls._file_list.append(app_conf.get_path())
        app_conf.set_loop_count(10)
        app_conf.append_region('dgemm', 28.0)
        app_conf.write()

        with open('test_policy.json', 'w') as ofile:
            ofile.write('{"POWER_PACKAGE_LIMIT_TOTAL": 200}')

        argv = ['dummy', geopm_test_launcher.detect_launcher(),
                '--geopm-report', cls._report_path,
                '--geopm-trace', cls._trace_path,
                '--geopm-agent', 'power_governor',
                #'--geopm-policy', 'test_policy.json',
                '--geopm-endpoint', cls._endpoint_path,
                '--geopm-trace-endpoint-policy', cls._endpoint_policy_trace,
                app_conf.get_exec_path()] + app_conf.get_exec_args()
        launcher = geopmpy.launcher.Factory().create(argv,
                                                     num_node=num_node,
                                                     num_rank=num_rank,
                                                     timeout=timeout)
        with open(test_name + '.log', 'a') as outfile:
            launcher.run(stdout=outfile, stderr=outfile)

        # app is done, can tear down daemon
        stdout, stderr = daemon.communicate(timeout=timeout)
        sys.stdout.write(stdout.decode())
        sys.stderr.write(stderr.decode())

    @classmethod
    def tearDownClass(cls):
        """If we are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset, clean up output.

        """
        if (sys.exc_info() == (None, None, None) and not
            cls._keep_files and not cls._skip_launch):
            for filename in cls._file_list:
                os.unlink(filename)
            for ff in glob.glob(cls._trace_path + '*'):
                os.unlink(ff)

    def setUp(self):
        pass

    def test_sample_log(self):
        # example the sample log produced by the daemon program
        pass

    def test_endpoint_policy_trace(self):
        # check that multiple different power caps appear in the policy trace
        pass

    def test_endpoint_trace_power(self):
        # check that observed power varies over time
        pass


if __name__ == '__main__':
    try:
        sys.argv.remove('--skip-launch')
        _g_skip_launch = True
    except ValueError:
        pass
    unittest.main()
