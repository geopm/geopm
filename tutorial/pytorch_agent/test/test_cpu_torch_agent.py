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
from apps.arithmetic_intensity import arithmetic_intensity
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
from experiment.monitor import monitor
from pytorch_experiment import cpu_torch
import process_cpu_frequency_sweep
from apps.minife import minife

import importlib
train_cpu_model = importlib.import_module("train_cpu_model-pytorch")

@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx2")
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx512")
@util.skip_unless_workload_exists("apps/minife/miniFE_openmp-2.0-rc3/src/miniFE.x")
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

        geopm_test_launcher.geopmwrite("MSR::PQR_ASSOC:RMID board 0 {}".format(0))
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:RMID board 0 {}".format(0))
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:EVENT_ID board 0 {}".format(2))

        cpu_max_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MAX_AVAIL board 0")
        uncore_max_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0")

        cpu_min_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MIN_AVAIL board 0")
        uncore_min_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0")

        cpu_base_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_STICKER board 0")

        node_count = 1

        ################
        # Monitor Runs #
        ################
        # MiniFE
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_activity_output/minife_monitor')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._minife_monitor_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )

        minife_app_conf = minife.MinifeAppConf(node_count)
        monitor.launch(app_conf=minife_app_conf, args=experiment_args,
                       experiment_cli_args=[])

        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_pytorch_output/aib_monitor')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._aib_monitor_dir = output_dir

        ranks_per_node = mach.num_core() - mach.num_package()
        ranks_per_package = ranks_per_node // mach.num_package()

        aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<26}',
             '--benchmarks=1 16'],
            mach,
            run_type='sse',
            ranks_per_node=ranks_per_node,
            distribute_slow_ranks=False)

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )

        monitor.launch(app_conf=aib_app_conf, args=experiment_args,
                       experiment_cli_args=[])

        ##################################
        # Uncore frequency sweep at base #
        ##################################
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_pytorch_output/aib_frequency_sweep')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._aib_uncore_freq_sweep_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            min_frequency=cpu_min_freq,
            max_frequency=cpu_max_freq,
            step_frequency=3e8,
            min_uncore_frequency=uncore_min_freq,
            max_uncore_frequency=uncore_max_freq,
            step_uncore_frequency=3e8,
            run_max_turbo=False
        )

        report_signals="CPU_POWER@package,DRAM_POWER,CPU_FREQUENCY_STATUS@package,CPU_CORE_TEMPERATURE@package,CPU_PACKAGE_TEMPERATURE@package,CPU_UNCORE_FREQUENCY_STATUS@package,MSR::QM_CTR_SCALED_RATE@package,INSTRUCTIONS_RETIRED@package,CYCLES_THREAD@package,CPU_ENERGY@package,MSR::APERF:ACNT@package,MSR::MPERF:MCNT@package,MSR::PPERF:PCNT@package,TIME@package,MSR::CPU_SCALABILITY_RATIO@package,DRAM_ENERGY"
        experiment_cli_args=['--geopm-report-signals={} --geopm-trace-signals={}'.format(report_signals, report_signals)]

        # We're reusing the AIB app conf from above here
        uncore_frequency_sweep.launch(app_conf=aib_app_conf, args=experiment_args,
                                      experiment_cli_args=experiment_cli_args)

        ##############
        # Parse data #
        ##############
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report', dir_name=cls._aib_uncore_freq_sweep_dir).get_df()
        host = df_frequency_sweep['host'][0]

        # Process the frequency sweep
        process_cpu_frequency_sweep.main(host, str(cls._aib_uncore_freq_sweep_dir.joinpath('process_aib_uncore_sweep')), [str(cls._aib_uncore_freq_sweep_dir)])
        aib_sweep_h5_path = str(cls._aib_uncore_freq_sweep_dir.joinpath('process_aib_uncore_sweep.h5'))

        # Train the cpu pytorch model
        pytorch_model = str(cls._aib_uncore_freq_sweep_dir.joinpath('cpu_control_integration.pt'))
        train_cpu_model.main(str(cls._aib_uncore_freq_sweep_dir.joinpath('process_aib_uncore_sweep.h5')), pytorch_model, None)

        # If the torch_root/lib is included before we do the pytorch training the python pytorch
        # libraries will throw an error).  As such we add this immediately before run.
        os.environ["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"] + ":" + os.environ["TORCH_ROOT"] + "/lib"
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_pytorch_output/aib_torch')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._aib_agent_dir = output_dir


        ranks_per_node = mach.num_core() - mach.num_package()
        ranks_per_package = ranks_per_node // mach.num_package()

        aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<26}',
             '--benchmarks=1 16'],
            mach,
            run_type='sse',
            ranks_per_node=ranks_per_node,
            distribute_slow_ranks=False)


        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            cpu_nn_path=pytorch_model,
        )

        cpu_torch.launch(app_conf=aib_app_conf, args=experiment_args,
                         experiment_cli_args=[])

        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_pytorch_output/minife_torch')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._minife_agent_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            cpu_nn_path=pytorch_model,
        )

        cpu_torch.launch(app_conf=minife_app_conf, args=experiment_args,
                         experiment_cli_args=[])

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_gpu_torch._keep_files = True

    def test_cpu_torch_aib(self):
        """
        AIB exhibits less energy consumption with the agent
        """
        df_monitor = geopmpy.io.RawReportCollection('*report', dir_name=self._aib_monitor_dir).get_app_df()
        monitor_runtime = df_monitor['runtime (s)'].mean()
        monitor_energy = df_monitor['package-energy (J)'].mean()

        df_agent = geopmpy.io.RawReportCollection('*report', dir_name=self._aib_agent_dir).get_app_df()
        for phi in sorted(set(df_agent['CPU_PHI'])):
            runtime = float(df_agent[df_agent['CPU_PHI'] == 0]['runtime (s)'].mean())
            energy = float(df_agent[df_agent['CPU_PHI'] == 0]['package-energy (J)'].mean())
            if phi > 0.0:
                self.assertLess(energy, monitor_energy);
                #TODO: An assertion about the runtime/fom impact should be made
                #self.assertGreaterEqual(runtime, monitor_runtime);


    def test_cpu_torch_minife(self):
        """
        MiniFE exhibits less energy consumption with the agent
        for all non-zero phi values.
        """
        monitor_fom = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_monitor_dir).get_app_df()['FOM'].mean()
        monitor_energy = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_monitor_dir).get_epoch_df()['package-energy (J)'].mean()

        df_agent = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_agent_dir).get_epoch_df()
        df_agent_app = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_agent_dir).get_app_df()

        for phi in set(df_agent['CPU_PHI']):
            fom = df_agent_app[df_agent_app['CPU_PHI'] == phi]['FOM'].mean()
            energy = df_agent[df_agent['CPU_PHI'] == phi]['package-energy (J)'].mean()
            if phi > 0.0:
                self.assertLess(energy, monitor_energy)
                util.assertNear(self, fom, monitor_fom)


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
