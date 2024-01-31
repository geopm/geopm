#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Test the scaling model region

"""


import sys
import re
import unittest
import os
import glob

import geopmpy.io
import geopmpy.agent
import geopmdpy.error
import geopmpy.hash
from integration.test import geopm_test_launcher
from integration.test import util


class AppConf(object):
    """Class that is used by the test launcher as a geopmpy.io.BenchConf
    when running the test_timed_scaling_region benchmark.

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
class TestIntegrationTimedsScalingRegion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_timed_scaling_region'
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_path = test_name + '-agent-config.json'
        # region_hash() of the sequence:
        # timed_scaling_region_0, timed_scaling_region_1, ... , timed_scaling_region_30
        cls._region_hash = [geopmpy.hash.hash_str('timed_scaling_region_{}'.format(ii))
                            for ii in range(31)]
        if not cls._skip_launch:
            num_node = util.get_num_node()
            num_rank = num_node
            # Set up agent configuration so that each region is assigned a different frequency
            freq_min = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
            freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
            freq_step = geopm_test_launcher.geopmread("CPUINFO::FREQ_STEP board 0")
            num_step = int((freq_sticker - freq_min) / freq_step + 0.5) + 1
            agent_conf_dict = {'FREQ_CPU_DEFAULT': freq_sticker}
            cls._region_freq = [freq_min + idx * freq_step
                                for idx in range(num_step)]
            freq_idx = 0
            for freq_idx in range(len(cls._region_freq)):
                agent_conf_dict['HASH_{}'.format(freq_idx)] = cls._region_hash[freq_idx]
                agent_conf_dict['FREQ_{}'.format(freq_idx)] = cls._region_freq[freq_idx]

            agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path,
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
                    util.assertNear(self, 1.0, report.get_field(region, 'runtime', 's'))

    @util.skip_unless_do_launch()
    def test_achieved_frequency(self):
        """Test that the reports show achieved frequencies near target
           frequency.

        """
        report = geopmpy.io.RawReport(self._report_path)
        host_names = report.host_names()
        for host in host_names:
            for rn in ['timed_scaling_region_{}'.format(idx)
                       for idx in range(len(self._region_freq))]:
                region = report.raw_region(host, rn)
                request_freq = report.get_field(region, 'frequency-map')
                actual_freq = report.get_field(region, 'frequency', 'Hz')
                util.assertNear(self, request_freq, actual_freq)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
