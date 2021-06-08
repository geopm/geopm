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

import geopm_context
import geopmpy.agent
import geopm_test_launcher
import util
import time
import geopmpy.topo


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
        #TODO: get num accelerators
        #TODO: verify they're level zero accels and not nvidia
        self._stdout = None
        self._stderr = None

    def tearDown(self):
        pass

    def test_power(self):
        #Query
        power = geopm_test_launcher.geopmread("LEVELZERO::POWER board_accelerator 0")
        power_limit_max = geopm_test_launcher.geopmread("LEVELZERO::POWER_LIMIT_MAX board_accelerator 0")
        power_limit_min = geopm_test_launcher.geopmread("LEVELZERO::POWER_LIMIT_MIN board_accelerator 0")
        power_limit_enabled_sustained = geopm_test_launcher.geopmread("LEVELZERO::POWER_LIMIT_ENABLED_SUSTAINED board_accelerator 0")
        power_limit_sustained = geopm_test_launcher.geopmread("LEVELZERO::POWER_LIMIT_SUSTAINED board_accelerator 0")
        power_limit_default = geopm_test_launcher.geopmread("LEVELZERO::POWER_LIMIT_DEFAULT board_accelerator 0")

        #Info
        sys.stdout.write("Power:\n");
        sys.stdout.write("\tPower: {}\n".format(power));
        sys.stdout.write("\tPower limit max: {}\n".format(power_limit_max));
        sys.stdout.write("\tPower limit min: {}\n".format(power_limit_min));
        sys.stdout.write("\tPower limit default: {}\n".format(power_limit_default));
        sys.stdout.write("\tPower limit sustained enable: {}\n".format(power_limit_enabled_sustained));
        sys.stdout.write("\tPower limit sustained: {}\n".format(power_limit_sustained));

        #Check
        self.assertGreater(power, 0)
        self.assertGreaterEqual(power, power_limit_min)

        self.assertLessEqual(power, power_limit_default)
        if(power_limit_max > 0): #Negative value indicates max is not supported
            self.assertLessEqual(power, power_limit_max)

        if(power_limit_enabled_sustained == 1):
            self.assertLessEqual(power, power_limit_sustained)

    def test_energy(self):
        #Query
        energy_prev = geopm_test_launcher.geopmread("LEVELZERO::ENERGY board_accelerator 0")
        energy_timestamp_prev = geopm_test_launcher.geopmread("LEVELZERO::ENERGY_TIMESTAMP board_accelerator 0")
        time.sleep(5)
        energy_curr = geopm_test_launcher.geopmread("LEVELZERO::ENERGY board_accelerator 0")
        energy_timestamp_curr = geopm_test_launcher.geopmread("LEVELZERO::ENERGY_TIMESTAMP board_accelerator 0")

        #Check
        self.assertNotEqual(energy_prev, energy_curr)
        self.assertNotEqual(energy_timestamp_prev, energy_timestamp_curr)

    def test_frequency(self):
        #Query
        standby_mode = geopm_test_launcher.geopmread("LEVELZERO::STANDBY_MODE board_accelerator 0")
        frequency_gpu = geopm_test_launcher.geopmread("LEVELZERO::FREQUENCY_GPU board_accelerator 0")
        frequency_min_gpu = geopm_test_launcher.geopmread("LEVELZERO::FREQUENCY_MIN_GPU board_accelerator 0")
        frequency_max_gpu = geopm_test_launcher.geopmread("LEVELZERO::FREQUENCY_MAX_GPU board_accelerator 0")
        frequency_range_min_gpu = geopm_test_launcher.geopmread("LEVELZERO::FREQUENCY_RANGE_MIN_GPU_CONTROL board_accelerator 0")
        frequency_range_max_gpu = geopm_test_launcher.geopmread("LEVELZERO::FREQUENCY_RANGE_MAX_GPU_CONTROL board_accelerator 0")

        #Info
        sys.stdout.write("Frequency:\n");
        sys.stdout.write("\tStandby Mode: {}\n".format(standby_mode));
        sys.stdout.write("\tFrequency GPU: {}\n".format(frequency_gpu));
        sys.stdout.write("\tFrequency GPU Min: {}\n".format(frequency_min_gpu));
        sys.stdout.write("\tFrequency GPU Max: {}\n".format(frequency_max_gpu));
        sys.stdout.write("\tFrequency GPU Control Min: {}\n".format(frequency_range_min_gpu));
        sys.stdout.write("\tFrequency GPU Control Max: {}\n".format(frequency_range_max_gpu));

        #Check
        if(standby_mode == 0): #We may enter idle and see 0 Hz
            self.assertGreaterEqual(frequency_gpu, 0)
        else:
            self.assertGreaterEqual(frequency_gpu, frequency_min_gpu)
            self.assertGreaterEqual(frequency_gpu, frequency_range_min_gpu)

        if(frequency_max_gpu > 0): #Negative value indicates max was not supported
            self.assertLessEqual(frequency_gpu, frequency_max_gpu)
            self.assertLessEqual(frequency_gpu, frequency_range_max_gpu)


if __name__ == '__main__':
    unittest.main()
