#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
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
from yaml import load
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader
from collections import defaultdict

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.energy_efficiency import gpu_activity
from apps.parres import parres

@util.skip_unless_do_launch()
@util.skip_unless_gpu()
@util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/dgemm-mpi-cublas")
@util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/nstream-mpi-cuda")
class TestIntegration_gpu_activity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        try:
            geopm_test_launcher.geopmread("NVML::GPU_UTILIZATION board 0") #Check NVML
            geopm_test_launcher.geopmread("DCGM::SM_ACTIVE board 0") #Check DCGM
            cls._do_use_nvidia = True
        except:
            cls._do_use_nvidia = False

        max_freq = geopm_test_launcher.geopmread("GPU_CORE_FREQUENCY_MAX_AVAIL board 0")

        # TODO:
        # Once the GPU Frequency sweep infrastructure is added a full
        # characterization integration test should be added that does
        # a GPU frequency sweep of parres dgemm, parses the frequency
        # sweep using the gen_gpu_activity_policy_recommendation.py
        # script, and uses the output provided.
        #
        # This is a less time consuming version of that approach,
        # where efficient freq is estimated based on min and max
        min_freq = geopm_test_launcher.geopmread("GPU_CORE_FREQUENCY_MIN_AVAIL board 0")
        efficient_freq = (min_freq + max_freq) / 2

        if cls._do_use_nvidia:
            #DGEMM
            output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_activity_output/dgemm')
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)
            mach = machine.init_output_dir(output_dir)

            cls._dgemm_output_dir = output_dir

            node_count = 1
            ranks_per_node = mach.num_core() - mach.num_package()
            ranks_per_package = ranks_per_node // mach.num_package()

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
                gpu_fe=efficient_freq,
                gpu_fmax=max_freq,
            )

            app_conf = parres.create_dgemm_appconf(mach, experiment_args)

            gpu_activity.launch(app_conf=app_conf, args=experiment_args,
                           experiment_cli_args=[])

            #STREAM
            output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_activity_output/stream')
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)
            mach = machine.init_output_dir(output_dir)

            cls._stream_output_dir = output_dir

            node_count = 1
            ranks_per_node = mach.num_core() - mach.num_package()
            ranks_per_package = ranks_per_node // mach.num_package()

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
                gpu_fe=efficient_freq,
                gpu_fmax=max_freq,
            )

            app_conf = parres.create_nstream_appconf(mach, experiment_args)

            gpu_activity.launch(app_conf=app_conf, args=experiment_args,
                           experiment_cli_args=[])

        else:
            #TODO: We don't have an Intel GPU dgemm style app integrated yet
            sys.exit(1)


    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_gpu_activity._keep_files = True

    def test_gpu_activity_dgemm(self):
        """
        PARRES DGEMM exhibits less energy consumption with the agent at phi > 50
        and FoM doesn't change significantly from phi 0 to phi 50
        """

        if self._do_use_nvidia:
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
        else:
            #TODO: We don't have an Intel GPU based test yet
            sys.stdout.write("Unsupported hardware, exiting\n");
            sys.exit(1)


    def test_gpu_activity_stream(self):
        """
        PARRES NSTREAM exhibits less energy consumption with the agent
        for all non-zero phi values..
        """

        if self._do_use_nvidia:
            # TODO: It would be preferable to use this,
            #       however it fails when parsing parres_nstream since
            #       parres_nstream does not have any regions
            #df = geopmpy.io.RawReportCollection('*report', dir_name=self._stream_output_dir).get_app_df()

            results = defaultdict(dict)
            for report_path in self._stream_output_dir.glob('*.report'):
                with open(report_path) as f:
                    report = load(f, Loader=SafeLoader)

                    # Store reports per phi value
                    results["{}".format(report['Policy'].get('GPU_PHI'))]['Figure of Merit'] = report['Figure of Merit']
                    host = list(report['Hosts'])[0]
                    results["{}".format(report['Policy'].get('GPU_PHI'))]['gpu-energy (J)'] = float(report['Hosts'][host]['Application Totals']['gpu-energy (J)'])

            #default_fom = results[0]['Figure of Merit']
            default_energy = results["0"]['gpu-energy (J)']
            for phi in results:
                if phi != "0":
                    energy = results[phi]['gpu-energy (J)']
                    self.assertLess(energy, default_energy);
        else:
            #TODO: We don't have an Intel GPU based test yet
            sys.stdout.write("Unsupported hardware, exiting\n");
            sys.exit(1)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
