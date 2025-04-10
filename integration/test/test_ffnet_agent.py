#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that ffnet agent is functional.
"""
import json
import sys
import unittest
import os
from pathlib import Path
import pandas as pd
import shutil
from types import SimpleNamespace

import geopmpy.agent
import geopmpy.io

from integration.apps.geopmbench import geopmbench
from integration.apps.parres import parres

from integration.test import util
from integration.test import geopm_test_launcher

from integration.experiment import machine
from integration.experiment.ffnet import ffnet

#@util.skip_unless_config_enable('beta')
#@util.skip_unless_do_launch()

class TestIntegration_ffnet_agent(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        mach = machine.init_output_dir('.')
        cls._skip_launch = not util.do_launch()
        cls._test_name = 'test_ffnet_nn_scripts'

        # Configure the ffnet agent
        cls._agent = 'ffnet'
        cls._perf_energy_bias = 0.5
        cls._ffnet_dir = Path('test_ffnet_output'))
        cls._cpu_nn_dummy_path = os.path.dirname(__file__) + "/ffnet_dummy.json"
        cls._cpu_fmap_dummy_path = os.path.dirname(__file__) + "/fmap_dummy.json"

        node_count = 1
        cls._run_count = 0

        # Setup Common Args
        ffnet_experiment_args = SimpleNamespace(
            output_dir=cls._ffnet_dir,
            perf_energy_bias=cls._perf_energy_bias,
            cpu_nn_path=cls._cpu_nn_dummy_path,
            cpu_freq_rec_path=cls._cpu_fmap_dummy_path,
            node_count=node_count,
            trial_count = 1,
            cool_off_time = 3,
        )

        experiment_cli_args=['--geopm-ctl=process']

        # Configure the CPU test application - geopmbench
        cpu_test_app_params = {
            'spin_bigo': 0.5,
            'sleep_bigo': 1.0,
            'dgemm_bigo': 1.0,
            'stream_bigo': 2.0,
            'loop_count': 2
        }
        cls._app_regions = {}
        #TODO: Get hashes later from a report and assemble this info
        cls._app_regions['cpu'] = {'spin':"geopmbench-0x120a248f",
                                   'sleep':"geopmbench-0x0f33c2ac",
                                   'dgemm':"geopmbench-0xa12de8ee",
                                   'stream':"geopmbench-0xf0e9be1c"}

        bench_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        bench_conf.set_loop_count(cpu_test_app_params['loop_count'])
        for region in cls._app_regions['cpu']:
            bench_conf.append_region(region, cpu_test_app_params[f"{region}_bigo"])
        bench_conf.write()

        ffnet_app_conf = geopmbench.GeopmbenchAppConf(os.path.abspath(bench_conf.get_path()), 1)

        os.environ["GEOPM_CPU_NN_PATH"] = cls._cpu_nn_dummy_path
        os.environ["GEOPM_CPU_FMAP_PATH"] = cls._cpu_fmap_dummy_path
        cls.launch_helper(ffnet, ffnet_experiment_args, ffnet_app_conf, experiment_cli_args)

        ###########
        # Helpers #
        ###########

    #Launch Helper for multiple job launches
    def launch_helper(self, experiment_type, experiment_args, app_conf, experiment_cli_args):
        if not self._skip_launch:
            self._run_count += 1

            output_dir = experiment_args.output_dir
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)

            experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)


    #Test region characterization for every line in trace
    #
    #For a given REGION_HASH, the probability of the correct IDd region is >95%
    #    at least 95% of the time
    def test_region_accuracy(self):

    #Test frequency selection
    #
    #For a given REGION_HASH, the average frequency is within 95% of the phi=0.5
    #    value
    def test_freq_selection(self):

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
