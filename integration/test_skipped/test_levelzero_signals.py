#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2024, Intel Corporation
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

    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER gpu 0")
    def test_power(self):
        #Query
        gpu_power = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER gpu 0")

        #Info
        sys.stdout.write("GPU Power:\n");
        sys.stdout.write("\tGPU Power: {}\n".format(gpu_power));

        #Check
        # Power should be positive and non-zero
        self.assertGreater(gpu_power, 0)

    #TODO: check that chip power is available, or skip?
    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_POWER gpu 0")
    def test_chip_power(self):
        #Query
        gpu_power = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER gpu 0")
        gpuchip_power = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_POWER gpu 0")

        #Info
        sys.stdout.write("Power:\n");
        sys.stdout.write("\tPackage Power: {}\n".format(gpu_power));
        sys.stdout.write("\tChip Aggregate Power: {}\n".format(gpuchip_power));

        # The package power should be greater than or equal to the sub-devices
        # on the package
        self.assertGreater(gpu_power, gpuchip_power)

    @util.skip_unless_geopmread("LEVELZERO::GPU_ENERGY gpu 0")
    def test_energy(self):
        sys.stdout.write("Running LevelZero Energy Test\n");
        #Query
        gpu_energy_prev = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY gpu 0")
        time.sleep(5)
        gpu_energy_curr = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY gpu 0")

        sys.stdout.write("Energy:\n");
        sys.stdout.write("\tEnergy Sample 0: {}\n".format(gpu_energy_prev));
        sys.stdout.write("\tEnergy Sample 1: {}\n".format(gpu_energy_curr));

        #Check
        self.assertNotEqual(gpu_energy_prev, gpu_energy_curr)

    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_ENERGY gpu 0")
    def test_chip_energy(self):
        sys.stdout.write("Running LevelZero GPU Chip Energy Test\n");
        #Query
        gpuchip_energy_prev = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_ENERGY gpu_chip 0")
        time.sleep(5)
        gpuchip_energy_curr = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_ENERGY gpu_chip 0")

        sys.stdout.write("GPU Chip Energy:\n");
        sys.stdout.write("\tGPU Chip Energy Sample 0: {}\n".format(gpuchip_energy_prev));
        sys.stdout.write("\tGPU Chip Energy Sample 1: {}\n".format(gpuchip_energy_curr));

        #Check
        self.assertNotEqual(gpuchip_energy_prev, gpuchip_energy_curr)

    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_FREQUENCY_STATUS gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_FREQUENCY_MIN_AVAIL gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_FREQUENCY_MAX_AVAIL gpu 0")
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

    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_RESET_COUNT gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_PROGRAMMING_ERRCOUNT gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_DRIVER_ERRCOUNT gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_COMPUTE_ERRCOUNT gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_NONCOMPUTE_ERRCOUNT gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_CACHE_ERRCOUNT gpu 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_CORE_RAS_DISPLAY_ERRCOUNT gpu 0")
    def test_ras(self):
        sys.stdout.write("Running LevelZero RAS Test\n");
        #Query
        gpu_ras_reset_count = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_RESET_COUNT gpu 0")
        gpu_ras_programming_errcount = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_PROGRAMMING_ERRCOUNT gpu 0")
        gpu_ras_driver_errcount = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_DRIVER_ERRCOUNT gpu 0")
        gpu_ras_compute_errcount = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_COMPUTE_ERRCOUNT gpu 0")
        gpu_ras_noncompute_errcount = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_NONCOMPUTE_ERRCOUNT gpu 0")
        gpu_ras_cache_errcount = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_CACHE_ERRCOUNT gpu 0")
        gpu_ras_display_errcount = geopm_test_launcher.geopmread("LEVELZERO::GPU_CORE_RAS_DISPLAY_ERRCOUNT gpu 0")

        #Info
        sys.stdout.write("RAS:\n");
        sys.stdout.write("\tGPU RAS Reset Count: {}\n".format(gpu_ras_reset_count));
        sys.stdout.write("\tGPU RAS Programming Errcount: {}\n".format(gpu_ras_programming_errcount));
        sys.stdout.write("\tGPU RAS Driver Errcount: {}\n".format(gpu_ras_driver_errcount));
        sys.stdout.write("\tGPU RAS Compute Errcount: {}\n".format(gpu_ras_compute_errcount));
        sys.stdout.write("\tGPU RAS NonCompute Errcount: {}\n".format(gpu_ras_noncompute_errcount));
        sys.stdout.write("\tGPU RAS Cache Errcount: {}\n".format(gpu_ras_cache_errcount));
        sys.stdout.write("\tGPU RAS Display Errcount: {}\n".format(gpu_ras_display_errcount));


if __name__ == '__main__':
    unittest.main()
