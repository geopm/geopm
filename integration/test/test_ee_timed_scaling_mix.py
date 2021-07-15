#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

"""Test the energy efficient agent by running a mix of frequency-
   sensitive and frequency-insensitive regions.

"""

from __future__ import absolute_import

import sys
import re
import unittest
import os
import glob

import geopm_context
import geopmpy.io
import geopmpy.agent
import geopmpy.error
import geopm_test_launcher
import util


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
        return os.path.join(script_dir, '.libs', 'test_ee_timed_scaling_mix')

    def get_exec_args(self):
        return []


@util.skip_unless_cpufreq()
@util.skip_unless_optimized()
@unittest.skip('Disabled pending overhaul of agent.')
class TestIntegrationEETimedScalingMix(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_ee_timed_scaling_mix'
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_path = test_name + '-agent-config.json'
        if not cls._skip_launch:
            num_node = 2
            num_rank = 2
            min_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
            sticker_freq = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
            agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path,
                                                 'energy_efficient',
                                                 {'frequency_min':min_freq,
                                                  'frequency_max':sticker_freq})
            launcher = geopm_test_launcher.TestLauncher(AppConf(),
                                                        agent_conf,
                                                        cls._report_path,
                                                        cls._trace_path,
                                                        time_limit=6000)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            launcher.run(test_name)


    def test_monotone_frequency(self):
        """Test that agent selects lower frequencies for regions with more
        stream.

        """
        report = geopmpy.io.RawReport(self._report_path)
        host_names = report.host_names()
        mix_name_re = re.compile(r'timed-[0-9]*\.[0-9]*-scaling-[0-9]*\.[0-9]*')
        num_re = re.compile(r'[0-9]*\.[0-9]*')
        for host_name in report.host_names():
            result = []
            for region_name in report.region_names(host_name):
                if mix_name_re.search(region_name):
                    fracs = num_re.findall(region_name)
                    stream_frac = float(fracs[0])
                    dgemm_frac = float(fracs[1])
                    region = report.raw_region(host_name, region_name)
                    self.assertTrue('requested-online-frequency' in region,
                                    msg='Learning for region {} did not complete'.format(region_name))
                    result.append((dgemm_frac, stream_frac, region['requested-online-frequency']))
            self.assertEqual(5, len(result))
            result.sort()
            last_freq = 0.0
            for (dgemm_frac, stream_frac, freq) in result:
                self.assertLessEqual(last_freq, freq,
                                     msg='Chosen frequency decreased with increasing dgemm fraction')
                last_freq = freq
            self.assertNotEqual(result[0][2], result[-1][2],
                                msg='Same frequency chosen for dgemm only and stream only.')

    def test_skip_network_regions(self):
        """Test that agent does not learn for regions declared as communication APIs.

        """
        report = geopmpy.io.RawReport(self._report_path)
        host_names = report.host_names()
        for host_name in report.host_names():
            found_barrier = False
            for region_name in report.region_names(host_name):
                if region_name == 'MPI_Barrier':
                    region = report.raw_region(host_name, region_name)
                    self.assertTrue('requested-online-frequency' not in region)
                    found_barrier = True
            self.assertTrue(found_barrier)


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
