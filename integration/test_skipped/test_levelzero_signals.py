#!/usr/bin/env python
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

from __future__ import absolute_import

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

        #TODO: Add query for numnber of devices and subdevices
        #TODO: Add check for ZES_SYSMAN_ENABLE=1

    def tearDown(self):
        pass

    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER board_accelerator 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL board_accelerator 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER_LIMIT_MAX_AVAIL board_accelerator 0")
    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER_LIMIT_DEFAULT board_accelerator 0")
    def test_power(self):
        #Query
        gpu_power = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER board_accelerator 0")
        gpu_power_limit_max = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER_LIMIT_MAX_AVAIL board_accelerator 0")
        gpu_power_limit_min = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL board_accelerator 0")
        gpu_power_limit_default = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER_LIMIT_DEFAULT board_accelerator 0")

        #Info
        sys.stdout.write("GPU Power:\n");
        sys.stdout.write("\tGPU Power: {}\n".format(gpu_power));
        sys.stdout.write("\tGPU Power limit max: {}\n".format(gpu_power_limit_max));
        sys.stdout.write("\tGPU Power limit min: {}\n".format(gpu_power_limit_min));

        #Check
        # Power should be positive and non-zero
        self.assertGreater(gpu_power, 0)
        # Power should be above min power limit
        self.assertGreaterEqual(gpu_power, gpu_power_limit_min)

        # Power should be below the TDP power limit
        self.assertLessEqual(gpu_power, gpu_power_limit_default)
        if(gpu_power_limit_max > 0): #Negative value indicates max is not supported
            # Power should be below the max power limit
            self.assertLessEqual(gpu_power, gpu_power_limit_max)

        #TODO: Power limit enabled sustained check

    #TODO: check that chip power is available, or skip?
    @util.skip_unless_geopmread("LEVELZERO::GPU_POWER board_accelerator 0")
    @util.skip_unless_geopmread("LEVELZERO::GPUCHIP_POWER board_accelerator 0")
    def test_chip_power(self):
        #Query
        gpu_power = geopm_test_launcher.geopmread("LEVELZERO::GPU_POWER board_accelerator 0")
        gpuchip_power = geopm_test_launcher.geopmread("LEVELZERO::GPUCHIP_POWER board_accelerator 0")

        #Info
        sys.stdout.write("Power:\n");
        sys.stdout.write("\tPackage Power: {}\n".format(gpu_power));
        sys.stdout.write("\tChip Aggregate Power: {}\n".format(gpuchip_power));

        # The package power should be greater than or equal to the sub-devices
        # on the package
        self.assertGreater(gpu_power, gpuchip_power)

    @util.skip_unless_geopmread("LEVELZERO::GPU_ENERGY board_accelerator 0")
    def test_energy(self):
        sys.stdout.write("Running LevelZero Energy Test\n");
        #Query
        gpu_energy_prev = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY board_accelerator 0")
        time.sleep(5)
        gpu_energy_curr = geopm_test_launcher.geopmread("LEVELZERO::GPU_ENERGY board_accelerator 0")

        sys.stdout.write("Energy:\n");
        sys.stdout.write("\tEnergy Sample 0: {}\n".format(gpu_energy_prev));
        sys.stdout.write("\tEnergy Sample 1: {}\n".format(gpu_energy_curr));

        #Check
        self.assertNotEqual(gpu_energy_prev, gpu_energy_curr)

    @util.skip_unless_geopmread("LEVELZERO::GPUCHIP_ENERGY board_accelerator 0")
    def test_chip_energy(self):
        sys.stdout.write("Running LevelZero GPU Chip Energy Test\n");
        #Query
        gpuchip_energy_prev = geopm_test_launcher.geopmread("LEVELZERO::GPUCHIP_ENERGY board_accelerator_chip 0")
        time.sleep(5)
        gpuchip_energy_curr = geopm_test_launcher.geopmread("LEVELZERO::GPUCHIP_ENERGY board_accelerator_chip 0")

        sys.stdout.write("GPU Chip Energy:\n");
        sys.stdout.write("\tGPU Chip Energy Sample 0: {}\n".format(gpuchip_energy_prev));
        sys.stdout.write("\tGPU Chip Energy Sample 1: {}\n".format(gpuchip_energy_curr));

        #Check
        self.assertNotEqual(gpuchip_energy_prev, gpuchip_energy_curr)

    @util.skip_unless_geopmread("LEVELZERO::GPUCHIP_FREQUENCY_STATUS board_accelerator 0")
    @util.skip_unless_geopmread("LEVELZERO::GPUCHIP_FREQUENCY_MIN_AVAIL board_accelerator 0")
    @util.skip_unless_geopmread("LEVELZERO::GPUCHIP_FREQUENCY_MAX_AVAIL board_accelerator 0")
    def test_frequency(self):
        sys.stdout.write("Running LevelZero Frequency Test\n");
        #Query
        frequency_gpu = geopm_test_launcher.geopmread("LEVELZERO::GPUCHIP_FREQUENCY_STATUS board_accelerator 0")
        gpu_min_frequency_limit = geopm_test_launcher.geopmread("LEVELZERO::GPUCHIP_FREQUENCY_MIN_AVAIL board_accelerator 0")
        gpu_max_frequency_limit = geopm_test_launcher.geopmread("LEVELZERO::GPUCHIP_FREQUENCY_MAX_AVAIL board_accelerator 0")

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
