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
        # Configure the ffnet agent
        cls._ffnet_policy = {'PERF_ENERGY_BIAS':0.5}
        cls._agent = 'ffnet'

        # Run FFNet agent using dummy net
        cls._nn_dummy_path = os.path.dirname(__file__) + "/ffnet_dummy.json"
        cls._fmap_dummy_path = os.path.dirname(__file__) + "/fmap_dummy.json"
        os.environ["GEOPM_CPU_NN_PATH"] = cls._cpu_nn_path
        os.environ["GEOPM_CPU_FMAP_PATH"] = cls._cpu_fmap_path
        cls.launch_helper(ffnet, ffnet_experiment_args, ffnet_app_conf, experiment_cli_args)

        # Run FFNet agent on geopmbench and parres
        for parres_app_conf in app_confs:
            cls.launch_helper(neural_net_sweep, gpu_experiment_args, parres_app_conf, [], None)

        # Run FFNet agent on workload of interest

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

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
