#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the ffnet agent is functional.
"""
import json
import sys
import unittest
import os

import geopmpy.agent
import geopmpy.io

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.ffnet import ffnet
from experiment.ffnet import neural_net_sweep
from experiment.ffnet import gen_hdf_from_fsweep
from experiment.ffnet import gen_region_parameters

from apps.arithmetic_intensity import arithmetic_intensity

@util.skip_unless_config_enable('beta')
@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")

class TestIntegration_ffnet(unittest.TestCas):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
    def test_ffnet_agent(self):
        #TODO: Run ffnet agent with dummy neural net and check for expected responses
        #      Dummy neural net can use region hash to ID region class definitively.

        #TODO: Run geopmbench on pkg0, ffnet phi=0 and phi=1
        #TODO:      and check that dgemm region is set to dgemm frequency
        #TODO:      and check that sleep region is set to sleep frequency

        #TODO [stretch]: Run geopmbench on pkg0 with dgemm/sleep, geopmbench on pkg1 with stream
        #TODO:           and check that apps are identified independently on each socket

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
