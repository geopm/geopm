#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the gpu_activity agent can improve
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

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.energy_efficiency import gpu_activity
from apps.parres import parres

@util.skip_unless_config_enable('beta')
@util.skip_unless_gpu()
@util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/")
class TestIntegration_gpu_activity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup DGEMM & STREAM applications, setup agent config, and execute.
        """
        cls._skip_launch = not util.do_launch()

        max_freq = geopm_test_launcher.geopmread("GPU_CORE_FREQUENCY_MAX_AVAIL board 0")

        # TODO:
        # Once the GPU Frequency sweep infrastructure is added a full
        # characterization integration test should be added that does
        # a GPU frequency sweep of parres dgemm, parses the frequency
        # sweep using the gen_gpu_activity_constconfig_recommendation.py
        # script, and uses the output provided.
        #
        # This is a less time consuming version of that approach,
        # where efficient freq is estimated based on min and max
        min_freq = geopm_test_launcher.geopmread("GPU_CORE_FREQUENCY_MIN_AVAIL board 0")
        efficient_freq = (min_freq + max_freq) / 2

        def launch_helper(experiment_type, experiment_args, app_conf, experiment_cli_args):
            if not cls._skip_launch:
                output_dir = experiment_args.output_dir
                if output_dir.exists() and output_dir.is_dir():
                    shutil.rmtree(output_dir)

                experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                       experiment_cli_args=experiment_cli_args)

        node_count=1
        mach = machine.init_output_dir('.')

        # DGEMM
        cls._dgemm_output_dir = Path(os.path.join('test_gpu_activity_output', 'dgemm'))
        experiment_args = SimpleNamespace(
            output_dir=cls._dgemm_output_dir,
            node_count=node_count,
            parres_cores_per_node=None,
            parres_gpus_per_node=None,
            parres_cores_per_rank=1,
            parres_init_setup=None,
            parres_exp_setup=None,
            parres_teardown=None,
            init_control=None,
            parres_args=None,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            phi_list=None,
        )

        if util.get_service_config_value('enable_nvml') == '1':
            app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                                    "apps/parres/Kernels/Cxx11/dgemm-mpi-cublas")
            app_conf = parres.create_dgemm_appconf_cuda(mach, experiment_args)
        elif util.get_service_config_value('enable_levelzero') == '1':
            app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                                    "apps/parres/Kernels/Cxx11/dgemm-onemkl")
            app_conf = parres.create_dgemm_appconf_oneapi(mach, experiment_args)
        if not os.path.exists(app_path):
            self.fail("Neither NVIDIA nor Intel dgemm variant was found")
        launch_helper(gpu_activity, experiment_args, app_conf, [])

        # STREAM
        cls._stream_output_dir = Path(os.path.join('test_gpu_activity_output', 'stream'))
        experiment_args.output_dir=cls._stream_output_dir
        experiment_args.parres_args="3 1000000000"

        if util.get_service_config_value('enable_nvml') == '1':
            app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                                    "apps/parres/Kernels/Cxx11/nstream-mpi-cuda")
            app_conf = parres.create_nstream_appconf_cuda(mach, experiment_args)
        elif util.get_service_config_value('enable_levelzero') == '1':
            app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                                    "apps/parres/Kernels/Cxx11/nstream-onemkl")
            app_conf = parres.create_nstream_appconf_oneapi(mach, experiment_args)
        if not os.path.exists(app_path):
            self.fail("Neither NVIDIA nor Intel dgemm variant was found")
        launch_helper(gpu_activity, experiment_args, app_conf, [])

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_gpu_activity._keep_files = True

    def test_gpu_activity_dgemm(self):
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
                self.assertLess(energy, default_energy)
                self.assertLess(fom, default_fom);

    def test_gpu_activity_stream(self):
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
