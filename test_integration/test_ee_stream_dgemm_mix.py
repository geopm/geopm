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

"""Test the energy efficient agent by doing a frequency stream/dgemm mix.

"""

import sys

import unittest
import os
import geopm_context
import geopmpy.io
import geopm_test_launcher

class AppConf(object):
    """Class that is used by the test launcher as a geopmpy.io.BenchConf
    when running the script as a benchmark.

    """
    def write(self):
        """No configuration files are required.

        """
        pass

    def get_exec_path(self):
        """Path to bencmark

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_ee_stream_dgemm_mix')

    def get_exec_args(self):
        return []


class TestIntegrationEEStreamDGEMMMix(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and store the output.

        """
        test_name = 'test_ee_stream_dgemm_mix'
        cls._num_node = 2
        cls._num_rank = 2
        app_conf = AppConf()
        agent_conf = geopmpy.io.AgentConf(test_name + '-agent-config.json', 'energy_efficient', {'frequency_min':1.0e9, 'frequency_max':2.1e9})
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, cls._report_path, cls._trace_path, time_limit=6000)
        cls._launcher.set_num_node(cls._num_node)
        cls._launcher.set_num_rank(cls._num_rank)
        cls._launcher.run(test_name)
        cls._output = geopmpy.io.AppOutput(cls._report_path, cls._trace_path + '*')

    @classmethod
    def tearDownClass(cls):
        """If we are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset, clean up output.

        """
        if (sys.exc_info() == (None, None, None) and
            os.getenv('GEOPM_KEEP_FILES') is None):
            cls._output.remove_files()
            cls._launcher.remove_files()

    def test_report_exists(self):
        """Test that a report is generated."""
        self.assertTrue(os.path.exists(self._report_path))

if __name__ == '__main__':
    unittest.main()
