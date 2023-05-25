#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that region classification neural nets and frequency
recommendation maps can be autogenerated and that the ffnet agent is functional.
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
    def test_neural_net_sweep(self):

        # Assign all cores to resource monitoring association ID 0. This
        # allows for monitoring the resource usage of all cores.
        geopm_test_launcher.geopmwrite("MSR::PQR_ASSOC:RMID board 0 {}".format(0))
        # Assign the resource monitoring ID for QM Events to match the per
        # core resource association ID above (0)
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:RMID board 0 {}".format(0))
        # Select monitoring event ID 0x2 - Total Memory Bandwidth Monitoring.
        # This is used to determine the Xeon Uncore utilization.
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:EVENT_ID board 0 {}".format(2))

        #TODO: Add cpu frequency sweep of aib and geopmbench

        #TODO: If on gpu-enabled system, add gpu frequency sweep of parres dgemm / stream

    def test_hdf_generation(self):
        #TODO: Add h5 generation
        #TODO:     and check for expected columns

    def test_nn_generation(self):
        #TODO: Add NN generation for CPU (and GPU if on gpu-enabled system)
        #          and check format / output columns

    def test_freqmap_generation(self):
        #TODO: Add region frequency map generation for CPU (and GPU if on gpu-enabled system)
        #TODO:     and check that frequency for stream at phi=1 is less than frequency for dgemm at phi=0 (in json)
        #TODO:     and check that frequency for dgemm at phi=0 >= frequency for dgemm at phi=1 (in json)

    def test_ffnet_agent(self):
        #TODO: Run geopmbench on pkg0, ffnet phi=0 and phi=1
        #TODO:     and check that perf is within 2% for phi=0
        #TODO:     and check perf @phi0 > perf @phi1
        #TODO:     and check total energy@phi0 > total energy@phi1

        #TODO: If on system with gpu, run parres dgemm ffnet with phi=0 and phi=1
        #TODO:     and check that perf is within 2% for phi=0
        #TODO:     and check perf0 > perf1
        #TODO:     and check gpu energy0 > gpu energy1
        #TODO:     and check that other gpus are not steered

        #TODO [stretch]: Try running different apps on different sockets/gpus and test that they are steered independently

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()