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
import glob
import os
from pathlib import Path
import pandas as pd
import numpy as np
import shutil
from types import SimpleNamespace

import geopmpy.agent
import geopmdpy.topo
import geopmpy.io

from integration.apps.geopmbench import geopmbench
from integration.apps.parres import parres

from integration.test import util
from integration.test import geopm_test_launcher

from integration.experiment import machine
from integration.experiment.ffnet import ffnet

@util.skip_unless_do_launch()

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
        cls._ffnet_dir = Path(os.path.join('test_ffnet_output', 'ffnet'))

        #TODO: Do a short geopmbench run and dynamically generate ffnet_dummy
        #      to use region hashes from the output (use get_region_map)
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
            gpu_nn_path=None,
            gpu_freq_rec_path=None,
            node_count=node_count,
            trial_count = 1,
            cool_off_time = 3,
        )

        experiment_cli_args=['--geopm-ctl=process']

        # Configure the CPU test application - geopmbench
        cls._loop_count = 2
        cls._test_app_params = {
            'spin': 0.5,
            'sleep': 1.0,
            'dgemm': 1.0,
            'stream': 2.0,
        }
        cls._app_regions = {}

        bench_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        bench_conf.set_loop_count(cls._loop_count)
        for region in cls._test_app_params:
            bench_conf.append_region(region, cls._test_app_params[region])
        bench_conf.write()

        ffnet_app_conf = geopmbench.GeopmbenchAppConf(os.path.abspath(bench_conf.get_path()), 1)

        os.environ["GEOPM_CPU_NN_PATH"] = cls._cpu_nn_dummy_path
        os.environ["GEOPM_CPU_FMAP_PATH"] = cls._cpu_fmap_dummy_path

        cls.launch_helper(cls, ffnet, ffnet_experiment_args, ffnet_app_conf, experiment_cli_args)

        # Get traces and reports
        cls._trace_path = glob.glob(experiment_args.output_dir + "/*trace*")
        cls._trace = geopmpy.io.AppOutput(traces=cls._trace_path[0])
        cls._trace_data = cls._trace.get_trace_data()

        cls._report_path = glob.glob(experiment_args.output_dir + "/*report*")
        cls._report_output = geopmpy.io.RawReport(cls._report_path[0])
        cls._app_regions = cls.get_region_map()

        # Get dummy ffnet and fmap
        cls._nn_dummy = self.get_json(open(cls._cpu_nn_dummy_path, "r"))
        cls._fmap_dummy = self.get_json(open(cls._cpu_fmap_dummy_path, "r"))

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

    #Get region name : region hash mapping
    def get_region_map(self):
        hostname = self._report_output.host_names()[0]
        region_names = report_output.region_names(hostname)
        region_map = {}

        for region in region_names:
            region_map[region] = report_output.raw_region(hostname, region)["hash"]

        return region_map

    #Check if JSON is valid (used in tests below)
    def get_json(self, json_file):
        try:
            json_parsed = json.load(json_file)
        except ValueError:
            json_parsed = None
        return json_parsed

    #Used to calculate region probabilities
    def sigmoid(x):
        return 1/ (1 + np.exp(-x))

    ###########
    #  Tests  #
    ###########

    # Test that we get a single report with our expected regions
    def test_single_report(self):
        self.assertEqual(len(cls._report_path), 1)

        for region in self._test_app_params:
            self.assertTrue(region in cls._app_regions)

    # Test that we get a single trace with expected FFNet trace columns
    def test_single_trace(self):
        self.assertEqual(len(cls._trace_path), 1)

        num_pkg = geopmdpy.topo.num_domain('package')
        for region in nn_dummy['trace_outputs']:
            self.assertEqual(cls._trace_data.columns.str.startswith("dgemm").sum(),
                             num_pkg)

    #Test region characterization for every line in trace
    #    For a given REGION_HASH, the probability of the correct IDd region is
    #    >95% at least 95% of the time
    def test_region_accuracy(self):
        # Calculate probabilities
        subset = cls._trace_data[list(cls._trace_data.filter(regex='geopmbench'))]
        probabilities = subset.apply(sigmoid)
        probabilities['REGION_HASH'] = cls._trace_data['REGION_HASH']

        # Count lines where probability of correct ID is >95%
        for region in cls._app_regions:
            region_hash = cls._app_regions[region]
            df = probabilities[probabilities["REGION_HASH"] == region_hash]
            samples_total = len(df)
            samples_good = len(df[df[f"geopmbench-{region_hash}_{geopmdpy.topo.DOMAIN_PACKAGE}_0"] > 0.95])
            self.assertTrue(samples_good/samples_total > 0.95)



    #Test frequency selection
    #    For a given REGION_HASH, the average frequency is within 95% of the
    #    phi=0.5 value
    #def test_freq_selection(self):

    ##TODO: Run at phi=0 vs phi=1, test correct frequency selection per region
if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
