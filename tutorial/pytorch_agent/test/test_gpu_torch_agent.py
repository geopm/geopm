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

from integration.test import util
from integration.test import geopm_test_launcher
from apps.parres import parres
from pytorch_experiment import gpu_torch
import process_gpu_frequency_sweep
from experiment.monitor import monitor
from experiment.gpu_frequency_sweep import gpu_frequency_sweep
import importlib
train_gpu_model = importlib.import_module("train_gpu_model-pytorch")

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
        # TODO: Implement a cleaner check for if TORCH_ROOT is set
        os.environ["TORCH_ROOT"]

        gpu_max_freq = geopm_test_launcher.geopmread("GPU_FREQUENCY_MAX_AVAIL board 0")
        cpu_max_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MAX_AVAIL board 0")
        uncore_max_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0")

        gpu_min_freq = geopm_test_launcher.geopmread("GPU_FREQUENCY_MIN_AVAIL board 0")
        cpu_min_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MIN_AVAIL board 0")
        uncore_min_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0")

        # TODO: test run GPU frequency sweep for PARRES DGEMM & PARRES NSTREAM after PR2024 is merged
        # TODO: test call process_gpu_frequency_sweep.py functions after PR2024 is merged
        # TODO: test call of train_gpu_model-pytorch.py functions  after PR2024 is merged

        ################
        # Monitor Runs #
        ################
        # STREAM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/stream_monitor')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._stream_monitor_dir = output_dir

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
        )

        stream_app_conf = parres.create_nstream_appconf(mach, experiment_args)

        monitor.launch(app_conf=stream_app_conf, args=experiment_args,
                       experiment_cli_args=[])

        # DGEMM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/dgemm_monitor')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._dgemm_monitor_dir = output_dir

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
        )

        dgemm_app_conf = parres.create_dgemm_appconf(mach, experiment_args)

        monitor.launch(app_conf=dgemm_app_conf, args=experiment_args,
                       experiment_cli_args=[])

        # DGEMM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/dgemm_gpu_freq_sweep')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._dgemm_gpu_freq_sweep_dir = output_dir

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
            min_frequency=cpu_max_freq,
            max_frequency=cpu_max_freq,
            step_frequency=1e8,
            min_uncore_frequency=uncore_max_freq,
            max_uncore_frequency=uncore_max_freq,
            step_uncore_frequency=1e8,
            min_gpu_frequency=gpu_min_freq,
            max_gpu_frequency=gpu_max_freq,
            step_uncore_frequency=1e8,
        )
        gpu_frequency_sweep.launch(app_conf=dgemm_app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)

        # STREAM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/stream_gpu_freq_sweep')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._stream_gpu_freq_sweep_dir = output_dir

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
            min_frequency=cpu_max_freq,
            max_frequency=cpu_max_freq,
            step_frequency=1e8,
            min_uncore_frequency=uncore_max_freq,
            max_uncore_frequency=uncore_max_freq,
            step_uncore_frequency=1e8,
            min_gpu_frequency=gpu_min_freq,
            max_gpu_frequency=gpu_max_freq,
            step_uncore_frequency=1e8,
        )
        gpu_frequency_sweep.launch(app_conf=stream_app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)


        ##############
        # Parse data #
        ##############
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report', dir_name=cls._dgemm_gpu_freq_sweep_dir).get_df()
        host = df_frequency_sweep['host'][0]

        # Process the frequency sweep
        process_gpu_frequency_sweep.main(host, str(cls._dgemm_gpu_freq_sweep_dir.joinpath('process_aib_uncore_sweep')), [str(cls._dgemm_gpu_freq_sweep_dir), str(cl._stream_gpu_freq_sweep_dir)])
        sweep_h5_path = str(cls._dgemm_gpu_freq_sweep_dir.joinpath('process_gpu_freq_sweep.h5'))

        # Train the cpu pytorch model
        pytorch_model = str(cls._dgemm_gpu_freq_sweep_dir.joinpath('gpu_control_integration.pt'))
        train_gpu_model.main(str(cls._dgemm_gpu_freq_sweep_dir.joinpath('process_gpu_freq_sweep.h5')), pytorch_model, None)

        # If the torch_root/lib is included before we do the pytorch training the python pytorch
        # libraries will throw an error).  As such we add this immediately before run.
        os.environ["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"] + ":" + os.environ["TORCH_ROOT"] + "/lib"
        #DGEMM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/dgemm_agent')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)

        cls._dgemm_agent_dir = output_dir

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
            gpu_nn_path=pytorch_model,
        )

        app_conf = parres.create_dgemm_appconf(mach, experiment_args)

        gpu_torch.launch(app_conf=app_conf, args=experiment_args,
                            experiment_cli_args=[])

        #STREAM
        output_dir = Path(__file__).resolve().parent.joinpath('test_gpu_torch_output/stream_agent')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)

        cls._stream_agent_dir = output_dir

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
            gpu_nn_path=pytorch_model,
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
        PARRES DGEMM exhibits less energy consumption with the agent
        for all phi values.
        """
        monitor_df = geopmpy.io.RawReportCollection('*report', dir_name=self._dgemm_monitor_dir).get_app_df()
        montor_fom = monitor_df['FOM'].mean()
        montor_energy = monitor_df['gpu-energy (J)'].mean()

        agent_df = geopmpy.io.RawReportCollection('*report', dir_name=self._dgemm_agent_dir).get_app_df()
        for phi in set(agent_df['GPU_PHI']):
            fom = float(agent_df[agent_df['GPU_PHI'] == phi]['FOM'])
            energy = float(agent_df[agent_df['GPU_PHI'] == phi]['gpu-energy (J)'])

            #TODO: Assert something more sensible about phi values, FOM, and energy
            self.assertLess(energy, monitor_energy)
            self.assertLess(fom, monitor_fom)

    @util.skip_unless_nvml()
    def test_gpu_torch_stream_nvidia(self):
        """
        PARRES NSTREAM exhibits less energy consumption with the agent
        for all phi values.
        """
        monitor_df = geopmpy.io.RawReportCollection('*report', dir_name=self._stream_monitor_dir).get_app_df()
        montor_fom = monitor_df['FOM'].mean()
        montor_energy = monitor_df['gpu-energy (J)'].mean()

        agent_df = geopmpy.io.RawReportCollection('*report', dir_name=self._stream_agent_dir).get_app_df()
        for phi in set(agent_df['GPU_PHI']):
            fom = float(agent_df[agent_df['GPU_PHI'] == phi]['FOM'])
            energy = float(agent_df[agent_df['GPU_PHI'] == phi]['gpu-energy (J)'])

            #TODO: Assert something more sensible about phi values, FOM, and energy
            self.assertLess(energy, monitor_energy);
            self.assertLess(fom, monitor_fom)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
