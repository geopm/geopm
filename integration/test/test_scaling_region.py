#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
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
        return os.path.join(script_dir, '.libs', 'test_scaling_region')

    def get_exec_args(self):
        return []


@util.skip_unless_cpufreq()
@util.skip_unless_optimized()
class TestIntegrationScalingRegion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_scaling_region'
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._skip_launch = not util.do_launch()
        cls._agent_conf_path = test_name + '-agent-config.json'
        # region_hash() of the sequence:
        # scaling_region_0, scaling_region_1, ... , scaling_region_30
        cls._region_hash = [geopmpy.hash.crc32_str('scaling_region_{}'.format(idx))
                            for idx in range(31)]
        if not cls._skip_launch:
            num_node = 1
            num_rank = 1
            # Set up agent configuration so that each region is assigned a different frequency
            freq_min = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
            freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
            freq_step = geopm_test_launcher.geopmread("CPUINFO::FREQ_STEP board 0")
            num_step = int((freq_sticker - freq_min) / freq_step + 0.5)
            agent_conf_dict = {'FREQ_CPU_DEFAULT':freq_sticker}
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


    def test_monotone_performance(self):
        """Test that the reports generated show linear performance with
           respect to CPU frequency.

        """
        report = geopmpy.io.RawReport(self._report_path)
        host_names = report.host_names()
        for host in host_names:
            region_names = [rn for rn in report.region_names(host)
                            if rn.startswith('scaling_region')]
            freq_time = []
            for rn in region_names:
                region = report.raw_region(host, rn)
                freq = report.get_field(region, 'frequency', 'Hz')
                time = report.get_field(region, 'runtime', 's')
                freq_time.append((freq, time))
            freq_time.sort()
            time = list(list(zip(*freq_time))[1])
            paired_time = zip(time[0:-1], time[1:])
            for pt in paired_time:
                self.assertGreater(pt[0], pt[1])

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
