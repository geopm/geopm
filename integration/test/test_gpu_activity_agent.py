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
import json
from pathlib import Path
from types import SimpleNamespace

import geopmpy.agent
import geopmpy.io

from integration.test import util as test_util
from apps.parres import parres
from experiment.energy_efficiency import gpu_activity
from experiment.gpu_ca_characterization import GPUCACharacterization
from experiment import util as exp_util

@test_util.skip_unless_gpu()
@test_util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/")
class TestIntegration_gpu_activity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup DGEMM & STREAM applications, setup agent config, and execute.
        """
        cls._skip_launch = not test_util.do_launch()

        def launch_helper(experiment_type, experiment_args, app_conf, experiment_cli_args):
            if not cls._skip_launch:
                exp_util.prep_experiment_output_dir(experiment_args.output_dir)

                experiment_cli_args.append('--geopm-ctl-local')
                experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                       experiment_cli_args=experiment_cli_args)

        base_dir = 'test_gpu_activity_output'
        cls._gpu_ca_characterization = GPUCACharacterization(base_dir=base_dir)
        mach = cls._gpu_ca_characterization.get_machine_config()
        gpu_config = {}
        if not cls._skip_launch:
            gpu_config = cls._gpu_ca_characterization.do_characterization()

        # Write config
        json_config = json.dumps(gpu_config, indent=4)
        with open("const_config_io-ca.json", "w") as outfile:
            outfile.write(json_config)

        const_path = Path('const_config_io-ca.json').resolve()
        os.environ["GEOPM_CONST_CONFIG_PATH"] = str(const_path)

        node_count=1

        # DGEMM GPU-CA Phi Sweep
        cls._dgemm_output_dir = Path(base_dir, 'dgemm')
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

        #if test_util.is_nvml_enabled():
        #    app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
        #                            "apps/parres/Kernels/Cxx11/dgemm-mpi-cublas")
        #    app_conf = parres.create_dgemm_appconf_cuda(mach, experiment_args)
        #elif test_util.is_oneapi_enabled():
        app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                "apps/parres/Kernels/Cxx11/dgemm-onemkl")
        app_conf = parres.create_dgemm_appconf_oneapi(mach, experiment_args)
        #if not os.path.exists(app_path):
        #    raise Exception("Neither NVIDIA or Intel dgemm variant was found")

        launch_helper(gpu_activity, experiment_args, app_conf, [])

        # STREAM
        cls._stream_output_dir = Path(base_dir, 'stream')
        experiment_args.output_dir = cls._stream_output_dir
        experiment_args.parres_args = "3 1000000000"

        #if test_util.is_nvml_enabled():
        #    app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
        #                            "apps/parres/Kernels/Cxx11/nstream-mpi-cuda")
        #    app_conf = parres.create_nstream_appconf_cuda(mach, experiment_args)
        #elif test_util.is_oneapi_enabled():
        app_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                                "apps/parres/Kernels/Cxx11/nstream-onemkl")
        app_conf = parres.create_nstream_appconf_oneapi(mach, experiment_args)
        #if not os.path.exists(app_path):
        #    raise Exception("Neither NVIDIA or Intel dgemm variant was found")

        launch_helper(gpu_activity, experiment_args, app_conf, [])

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_gpu_activity._keep_files = True

    def test_gpu_activity_dgemm(self):
        """
        PARRES DGEMM exhibits less energy consumption with the agent at phi > 50
        and FoM doesn't change significantly from phi 0 to phi 50
        """

        df = geopmpy.io.RawReportCollection('*report*', dir_name=self._dgemm_output_dir).get_app_df()

        default_fom = float(df[df['GPU_PHI'] == 0]['FOM'])
        default_energy = float(df[df['GPU_PHI'] == 0]['gpu-energy (J)'])

        for phi in df['GPU_PHI']:
            fom = float(df[df['GPU_PHI'] == phi]['FOM'])
            energy = float(df[df['GPU_PHI'] == phi]['gpu-energy (J)'])
            if phi <= 0.5:
                test_util.assertNear(self, fom, default_fom)
            elif phi > 0.5:
                self.assertLess(energy, default_energy)

    def test_gpu_activity_stream(self):
        """
        PARRES NSTREAM exhibits less energy consumption with the agent
        for all non-zero phi values.
        """
        df = geopmpy.io.RawReportCollection('*report*', dir_name=self._stream_output_dir).get_app_df()

        default_fom = float(df[df['GPU_PHI'] == 0]['FOM'])
        default_energy = float(df[df['GPU_PHI'] == 0]['gpu-energy (J)'])

        for phi in df['GPU_PHI']:
            fom = float(df[df['GPU_PHI'] == phi]['FOM'])
            energy = float(df[df['GPU_PHI'] == phi]['gpu-energy (J)'])
            if phi != 0:
                self.assertLess(energy, default_energy);

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    test_util.do_launch()
    unittest.main()
