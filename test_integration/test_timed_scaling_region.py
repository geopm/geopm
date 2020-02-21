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

"""Test the scaling model region

"""

from __future__ import absolute_import

import sys
import re
import unittest
import os
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_context
import geopmpy.io
from test_integration import geopm_test_launcher
from test_integration import util

_g_skip_launch = False

class AppConf(object):
    """Class that is used by the test launcher as a geopmpy.io.BenchConf
    when running the test_scaling_region benchmark.

    """
    def write(self):
        """No configuration files are required.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_timed_scaling_region')

    def get_exec_args(self):
        return []


@util.skip_unless_cpufreq()
@util.skip_unless_optimized()
@util.skip_unless_run_long_tests()
class TestIntegrationScalingRegion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_timed_scaling_region'
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._skip_launch = _g_skip_launch
        cls._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None
        cls._agent_conf_path = test_name + '-agent-config.json'
        # region_hash() of the sequence:
        # timed_scaling_region_0, timed_scaling_region_1, ... , timed_scaling_region_30
        cls._region_hash = [0xb55b9ce7, 0x101a0e99, 0xfa34ceea, 0x5f755c94,
                            0x2b8538fd, 0x8ec4aa83, 0x64ea6af0, 0xc1abf88e,
                            0x8d0aa222, 0x284b305c, 0x50541c1a, 0x43f6846d,
                            0x77112cf4, 0x64b3b483, 0x1ede7dc6, 0x0d7ce5b1,
                            0x399b4d28, 0x2a39d55f, 0xcd40dfa2, 0xdee247d5,
                            0xba7adc69, 0xa9d8441e, 0x9d3fec87, 0x8e9d74f0,
                            0xf4f0bdb5, 0xe75225c2, 0xd3b58d5b, 0xc017152c,
                            0x276e1fd1, 0x34cc87a6, 0x1f3b4e17]
        if not cls._skip_launch:
            num_node = 1
            num_rank = 1
            # Set up agent configuration so that each region is assigned a different frequency
            freq_min = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
            freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
            freq_step = geopm_test_launcher.geopmread("CPUINFO::FREQ_STEP board 0")
            num_step = int((freq_sticker - freq_min) / freq_step + 0.5)
            agent_conf_dict = {'FREQ_MIN':freq_min,
                               'FREQ_MAX':freq_sticker}
            cls._region_freq = [freq_min + idx * freq_step
                                for idx in range(num_step)]
            freq_idx = 0
            for freq_idx in range(len(cls._region_freq)):
                agent_conf_dict['HASH_{}'.format(freq_idx)] = cls._region_hash[freq_idx]
                agent_conf_dict['FREQ_{}'.format(freq_idx)] = cls._region_freq[freq_idx]

            agent_conf = geopmpy.io.AgentConf(cls._agent_conf_path,
                                              'frequency_map',
                                              agent_conf_dict)
            launcher = geopm_test_launcher.TestLauncher(AppConf(),
                                                        agent_conf,
                                                        cls._report_path,
                                                        cls._trace_path,
                                                        time_limit=6000)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            launcher.run(test_name)

    @classmethod
    def tearDownClass(cls):
        """If we are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset, clean up output.

        """
        if (sys.exc_info() == (None, None, None) and not
            cls._keep_files and not cls._skip_launch):
            os.unlink(cls._agent_conf_path)
            os.unlink(cls._report_path)

    def assertNear(self, a, b, epsilon=0.05, msg=''):
        denom = a if a != 0 else 1
        if abs((a - b) / denom) >= epsilon:
            self.fail('The fractional difference between {a} and {b} is greater than {epsilon}.  {msg}'.format(a=a, b=b, epsilon=epsilon, msg=msg))

    def test_uniform_performance(self):
        """Test that the reports generated show uniform performance with
           respect to CPU frequency.

        """
        report = geopmpy.io.RawReport(self._report_path)
        host_names = report.host_names()
        for host in host_names:
            for rn in report.region_names(host):
                if rn.startswith('timed_scaling_region'):
                    region = report.raw_region(host, rn)
                    self.assertNear(1.0, report.get_field(region, 'runtime', 'sec'))

if __name__ == '__main__':
    try:
        sys.argv.remove('--skip-launch')
        _g_skip_launch = True
    except ValueError:
        pass
    unittest.main()
