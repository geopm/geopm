#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the cpu characterization approach
and related scripts function to create a CPU-CA characterization file with legal values.
"""

import json
import sys
import unittest
import os
from pathlib import Path

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.cpu_ca_characterization import CPUCACharacterization

@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")
class TestIntegration_cpu_characterization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        base_dir = 'test_cpu_characterization_output'
        cls._cpu_ca_characterization = CPUCACharacterization(full_characterization=False,
                                                             base_dir=base_dir)

        # Grabbing system frequency parameters for experiment frequency bounds
        cls._cpu_base_freq = cls._cpu_ca_characterization.get_cpu_base_freq()
        cls._cpu_max_freq = cls._cpu_ca_characterization.get_cpu_max_freq()
        cls._cpu_min_freq = cls._cpu_ca_characterization.get_cpu_min_freq()
        cls._uncore_max_freq = cls._cpu_ca_characterization.get_uncore_max_freq()
        cls._uncore_min_freq = cls._cpu_ca_characterization.get_uncore_min_freq()

        #####################
        # Setup Common Args #
        #####################
        freq_cfg = """
        CPU_FREQUENCY_MAX_CONTROL board 0 {}
        CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 {}
        CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 {}
        """.format(cls._cpu_max_freq, cls._uncore_max_freq, cls._uncore_min_freq)

        cpu_ca_config = cls._cpu_ca_characterization.do_characterization(freq_cfg)

        json_config = json.dumps(cpu_ca_config, indent=4)
        cc_file_name = 'const_config_io-characterization.json'
        with open(cc_file_name, "w") as outfile:
            outfile.write(json_config)

        cls._const_path = Path(cc_file_name).resolve()
        os.environ['GEOPM_CONST_CONFIG_PATH'] = str(cls._const_path)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_cpu_characterization._keep_files = True

    def test_const_config_file_exists(self):
        """
        Basic testing of file creation
        """

        if not self._const_path.exists():
            raise Exception("Error: <geopm> test_cpu_characterization.py: Creation of "
                            "ConstConfigIOGroup configuration file failed")

    def test_const_config_signals(self):
        """
        Basic testing of signal availability and within system ranges
        """

        # Check that we can read constconfig
        cpu_freq_efficient = geopm_test_launcher.geopmread("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY board 0")
        uncore_freq_efficient = geopm_test_launcher.geopmread("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY board 0")

        # Check Fce within range
        self.assertGreaterEqual(cpu_freq_efficient, self._cpu_min_freq)
        self.assertLessEqual(cpu_freq_efficient, self._cpu_max_freq)

        # Check Fue within range
        self.assertGreaterEqual(uncore_freq_efficient, self._uncore_min_freq)
        self.assertLessEqual(uncore_freq_efficient, self._uncore_max_freq)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
