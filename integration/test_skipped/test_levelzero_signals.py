#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import os
import sys
import unittest
import subprocess
import io
import json
import util
import time

import geopmpy.agent
import geopmdpy.topo
from integration.test import geopm_test_launcher


@util.skip_unless_levelzero()
@util.skip_unless_levelzero_power()
class TestIntegrationLevelZeroSignals(unittest.TestCase):
    """Test the levelzero signals

    """
    @classmethod
    def setUpClass(cls):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._app_exec_path = os.path.join(script_dir, '.libs', 'test_levelzero_signals')

    def setUp(self):
        self._stdout = None
        self._stderr = None

        #TODO: Add query for number of devices and subdevices
        #TODO: Add check for ZES_SYSMAN_ENABLE=1

    def tearDown(self):
        pass

    def test_power(self):
        #Query
        power = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER gpu 0")
        power_limit_max = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER_LIMIT_MAX_AVAIL gpu 0")
        power_limit_min = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL gpu 0")

        #Info
        sys.stdout.write("Power:\n");
        sys.stdout.write("\tPower: {}\n".format(power));
        sys.stdout.write("\tPower limit max: {}\n".format(power_limit_max));
        sys.stdout.write("\tPower limit min: {}\n".format(power_limit_min));

        #Check
        self.assertGreater(power, 0)
        self.assertGreaterEqual(power, power_limit_min)

        self.assertLessEqual(power, power_limit_default)
        if(power_limit_max > 0): #Negative value indicates max is not supported
            self.assertLessEqual(power, power_limit_max)

        #TODO: Power limit enabled sustained check

    def test_energy(self):
        sys.stdout.write("Running LevelZero Energy Test\n");
        #Query
        energy_prev = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY gpu 0")
        energy_timestamp_prev = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY_TIMESTAMP gpu 0")
        time.sleep(5)
        energy_curr = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY gpu 0")
        energy_timestamp_curr = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY_TIMESTAMP gpu 0")

        sys.stdout.write("Energy:\n");
        sys.stdout.write("\tEnergy Sample 0: {}\n".format(energy_prev));
        sys.stdout.write("\tEnergy Sample 1: {}\n".format(energy_curr));

        #Check
        self.assertNotEqual(energy_prev, energy_curr)
        self.assertNotEqual(energy_timestamp_prev, energy_timestamp_curr)

    def test_frequency(self):
        sys.stdout.write("Running LevelZero Frequency Test\n");
        #Query
        frequency_gpu = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_FREQUENCY_STATUS gpu 0")
        gpu_min_frequency_limit = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_FREQUENCY_MIN_AVAIL gpu 0")
        gpu_max_frequency_limit = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_FREQUENCY_MAX_AVAIL gpu 0")

        #Info
        sys.stdout.write("Frequency:\n");
        sys.stdout.write("\tFrequency GPU: {}\n".format(frequency_gpu));
        sys.stdout.write("\tFrequency GPU Min Limit: {}\n".format(gpu_min_frequency_limit));
        sys.stdout.write("\tFrequency GPU Max Limit: {}\n".format(gpu_max_frequency_limit));

        #TODO: standby mode check
        self.assertGreaterEqual(frequency_gpu, gpu_min_frequency_limit)
        if(gpu_max_frequency_limit > 0): #Negative value indicates max was not supported
            self.assertLessEqual(frequency_gpu, gpu_max_frequency_limit)


if __name__ == '__main__':
    unittest.main()
