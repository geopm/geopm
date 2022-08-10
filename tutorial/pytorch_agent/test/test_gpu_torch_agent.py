#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the gpu_torch agent can improve
efficiency of an application.
"""
import sys
import unittest
import os
from pathlib import Path
import shutil
from experiment import machine
from types import SimpleNamespace

import geopmpy.agent
import geopmpy.io
from yaml import load
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader
from collections import defaultdict

from integration.test import util
from integration.test import geopm_test_launcher
from apps.parres import parres
from pytorch_experiment import gpu_torch

@util.skip_unless_do_launch()
@util.skip_unless_gpu()
@util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/dgemm-mpi-cublas")
@util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/nstream-mpi-cuda")
class TestIntegration_gpu_torch(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        # TODO: check agent exists
        # TODO: check GEOPM_PLUGIN_PATH

        # TODO: run GPU frequency sweep for PARRES DGEMM & PARRES NSTREAM
        # TODO: call process_gpu_frequency_sweep.py functions
        # TODO: call train_gpu_model-pytorch.py functions

        path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
               'gpu_control.pt')

        #DGEMM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/dgemm')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)

        cls._dgemm_output_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=1,
            parres_cores_per_node=None,
            parres_gpus_per_node=None,
            parres_cores_per_rank=1,
            parres_init_setup=None,
            parres_exp_setup=None,
            parres_teardown=None,
            parres_args=None,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            gpu_nn_path=path,
        )

        app_conf = parres.create_dgemm_appconf(mach, experiment_args)

        gpu_torch.launch(app_conf=app_conf, args=experiment_args,
                            experiment_cli_args=[])

        #STREAM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/stream')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)

        cls._stream_output_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=1,
            parres_cores_per_node=None,
            parres_gpus_per_node=None,
            parres_cores_per_rank=1,
            parres_init_setup=None,
            parres_exp_setup=None,
            parres_teardown=None,
            parres_args=None,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            gpu_nn_path=None,
        )

        app_conf = parres.create_nstream_appconf(mach, experiment_args)

        gpu_torch.launch(app_conf=app_conf, args=experiment_args,
                            experiment_cli_args=[])


    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_gpu_torch._keep_files = True

    @util.skip_unless_nvml()
    def test_gpu_torch_dgemm_nvidia(self):
        """
        PARRES DGEMM exhibits less energy consumption with the agent at phi > 50
        and FoM doesn't change significantly from phi 0 to phi 50
        """
        df = geopmpy.io.RawReportCollection('*report', dir_name=self._dgemm_output_dir).get_app_df()

        default_fom = float(df[df['GPU_PHI'] == 0]['FOM'])
        default_energy = float(df[df['GPU_PHI'] == 0]['gpu-energy (J)'])

        for phi in df['GPU_PHI']:
            fom = float(df[df['GPU_PHI'] == phi]['FOM'])
            energy = float(df[df['GPU_PHI'] == phi]['gpu-energy (J)'])
            if phi <= 0.5:
                util.assertNear(self, fom, default_fom)
            elif phi > 0.5:
                self.assertLess(energy, default_energy);
                self.assertLess(fom, default_fom);

    @util.skip_unless_nvml()
    def test_gpu_torch_stream_nvidia(self):
        """
        PARRES NSTREAM exhibits less energy consumption with the agent
        for all non-zero phi values.
        """
        df = geopmpy.io.RawReportCollection('*report', dir_name=self._stream_output_dir).get_app_df()

        default_fom = float(df[df['GPU_PHI'] == 0]['FOM'])
        default_energy = float(df[df['GPU_PHI'] == 0]['gpu-energy (J)'])

        for phi in df['GPU_PHI']:
            fom = float(df[df['GPU_PHI'] == phi]['FOM'])
            energy = float(df[df['GPU_PHI'] == phi]['gpu-energy (J)'])
            if phi != 0:
                self.assertLess(energy, default_energy);

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
