#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the cpu_activity agent can improve
the energy efficiency of an application.
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
from experiment.energy_efficiency import cpu_activity
from experiment.monitor import monitor
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
from experiment.uncore_frequency_sweep import gen_cpu_activity_policy_recommendation
from apps.arithmetic_intensity import arithmetic_intensity
from apps.minife import minife

@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx2")
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx512")
@util.skip_unless_workload_exists("apps/minife/miniFE_openmp-2.0-rc3/src/miniFE.x")
class TestIntegration_cpu_activity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """

        geopm_test_launcher.geopmwrite("MSR::PQR_ASSOC:RMID board 0 {}".format(0))
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:RMID board 0 {}".format(0))
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:EVENT_ID board 0 {}".format(2))

        cpu_base_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_STICKER board 0")
        cpu_max_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MAX_AVAIL board 0")
        uncore_max_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0")

        cpu_min_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MIN_AVAIL board 0")
        uncore_min_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0")

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
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )

        minife_app_conf = minife.MinifeAppConf(node_count)
        monitor.launch(app_conf=minife_app_conf, args=experiment_args,
                       experiment_cli_args=[])

        # Arithmetic Intensity Benchmark
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_activity_output/aib_monitor')
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
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_activity_output/uncore_frequency_sweep')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._aib_uncore_freq_sweep_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            min_frequency=cpu_base_freq, #TODO: base or max?
            max_frequency=cpu_base_freq, #TODO: base or max?
            step_frequency=1e8,
            min_uncore_frequency=uncore_min_freq,
            max_uncore_frequency=uncore_max_freq,
            step_uncore_frequency=1e8,
            run_max_turbo=False
        )
        report_signals="MSR::QM_CTR_SCALED_RATE@package,MSR::UNCORE_PERF_STATUS:FREQ@package,MSR::CPU_SCALABILITY_RATIO@package,CPU_FREQUENCY_CONTROL@package,MSR::UNCORE_RATIO_LIMIT:MIN_RATIO@package,MSR::UNCORE_RATIO_LIMIT:MAX_RATIO@package"
        experiment_cli_args=['--geopm-report-signals={}'.format(report_signals)]

        # We're reusing the AIB app conf from above here
        uncore_frequency_sweep.launch(app_conf=aib_app_conf, args=experiment_args,
                                      experiment_cli_args=experiment_cli_args)

        ##############
        # Parse data #
        ##############
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report', dir_name=cls._aib_uncore_freq_sweep_dir).get_df()
        policy = gen_cpu_activity_policy_recommendation.main(df_frequency_sweep, ['intensity_1', 'intensity_16'])

        uncore_efficient_freq = policy['CPU_UNCORE_FREQ_EFFICIENT']
        uncore_mbm_list = []

        for key,value in policy.items():
            if key not in ("CPU_FREQ_MAX", "CPU_FREQ_EFFICIENT", "CPU_UNCORE_FREQ_MAX", "CPU_UNCORE_FREQ_EFFICIENT", "CPU_PHI"):
                uncore_mbm_list.append(value)

        #######################################################
        # Core frequency sweep at fixed uncore_efficient_freq #
        #######################################################
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_activity_output/core_frequency_sweep')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._aib_core_freq_sweep_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            min_frequency=cpu_min_freq,
            max_frequency=cpu_max_freq,
            step_frequency=1e8,
            min_uncore_frequency=uncore_efficient_freq,
            max_uncore_frequency=uncore_efficient_freq,
            step_uncore_frequency=1e8,
            run_max_turbo=False
        )
        report_signals="MSR::QM_CTR_SCALED_RATE@package,MSR::UNCORE_PERF_STATUS:FREQ@package,MSR::CPU_SCALABILITY_RATIO@package,CPU_FREQUENCY_CONTROL@package,MSR::UNCORE_RATIO_LIMIT:MIN_RATIO@package,MSR::UNCORE_RATIO_LIMIT:MAX_RATIO@package"
        experiment_cli_args=['--geopm-report-signals={}'.format(report_signals)]

        uncore_frequency_sweep.launch(app_conf=aib_app_conf, args=experiment_args,
                                      experiment_cli_args=experiment_cli_args)

        ##############
        # Parse data #
        ##############
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report', dir_name=cls._aib_core_freq_sweep_dir).get_df()
        policy = gen_cpu_activity_policy_recommendation.main(df_frequency_sweep, ['intensity_1', 'intensity_16'])
        cpu_efficient_freq = policy['CPU_FREQ_EFFICIENT']

        ################################
        # CPU Activity Agent phi sweep #
        ################################

        # Arithmetic Intensity Benchmark
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_activity_output/aib_cpu_activity')
        if output_dir.exists() and output_dir.is_dir():
            shutil.rmtree(output_dir)
        mach = machine.init_output_dir(output_dir)
        cls._aib_agent_dir = output_dir

        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            cpu_fe=cpu_efficient_freq,
            cpu_fmax=cpu_max_freq,
            uncore_fe=cpu_efficient_freq,
            uncore_fmax=cpu_max_freq,
            uncore_mbm_list=uncore_mbm_list,
            phi_list=None,
        )

        cpu_activity.launch(app_conf=aib_app_conf, args=experiment_args,
                            experiment_cli_args=[])

        # MiniFE
        output_dir = Path(__file__).resolve().parent.joinpath('test_cpu_activity_output/minife_cpu_activity')
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
            cpu_fe=cpu_efficient_freq,
            cpu_fmax=cpu_max_freq,
            uncore_fe=cpu_efficient_freq,
            uncore_fmax=cpu_max_freq,
            uncore_mbm_list=uncore_mbm_list,
            phi_list=None,
        )

        app_conf = minife.MinifeAppConf(node_count)

        cpu_activity.launch(app_conf=minife_app_conf, args=experiment_args,
                            experiment_cli_args=[])

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_cpu_activity._keep_files = True

    def test_cpu_activity_aib(self):
        """
        AIB testing to make sure agent tuning based on AIB yielded sensible
        results
        """
        df_monitor = geopmpy.io.RawReportCollection('*report', dir_name=self._aib_monitor_dir).get_df()
        monitor_runtime_16 = df_monitor[df_monitor['region'] == 'intensity_16']['runtime (s)'].mean()
        monitor_energy_16 = df_monitor[df_monitor['region'] == 'intensity_16']['package-energy (J)'].mean()

        monitor_runtime_1 = df_monitor[df_monitor['region'] == 'intensity_1']['runtime (s)'].mean()
        monitor_energy_1 = df_monitor[df_monitor['region'] == 'intensity_1']['package-energy (J)'].mean()

        df_agent = geopmpy.io.RawReportCollection('*report', dir_name=self._aib_agent_dir).get_df()
        df_agent_16 = df_agent[df_agent['region'] == 'intensity_16']
        df_agent_1 = df_agent[df_agent['region'] == 'intensity_1']

        # Test FoM & Runtime impact on the compute intensive workload
        # used for tuning the agent
        for phi in set(df_agent_16['CPU_PHI']):
            runtime = df_agent_16[df_agent_16['CPU_PHI'] == phi]['runtime (s)'].mean()
            energy = df_agent_16[df_agent_16['CPU_PHI'] == phi]['package-energy (J)'].mean()
            if phi <= 0.5:
                util.assertNear(self, runtime, monitor_runtime_16)
                util.assertNear(self, energy, monitor_energy_16)
            if phi >= 0.5:
                self.assertGreaterEqual(runtime, monitor_runtime_16)

        # Test FoM & Runtime impact on the memory intensive workload
        # used for tuning the agent
        for phi in set(df_agent_1['CPU_PHI']):
            runtime = df_agent_1[df_agent_1['CPU_PHI'] == phi]['runtime (s)'].mean()
            energy = df_agent_1[df_agent_1['CPU_PHI'] == phi]['package-energy (J)'].mean()

            if phi <= 0.5:
                util.assertNear(self, runtime, monitor_runtime_1)
            if phi >= 0.5:
                self.assertLessEqual(energy, monitor_energy_1)

    def test_cpu_activity_minife(self):
        """
        MiniFE testing to make sure agent saves energy for cpu frequency
        insensitive workloads
        """
        monitor_fom = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_monitor_dir).get_app_df()['FOM'].mean()
        monitor_energy = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_monitor_dir).get_epoch_df()['package-energy (J)'].mean()

        df_agent = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_agent_dir).get_epoch_df()
        df_agent_app = geopmpy.io.RawReportCollection('*report', dir_name=self._minife_agent_dir).get_app_df()

        for phi in set(df_agent['CPU_PHI']):
            fom = df_agent_app[df_agent_app['CPU_PHI'] == phi]['FOM'].mean()
            energy = df_agent[df_agent['CPU_PHI'] == phi]['package-energy (J)'].mean()

            if phi < 0.5:
                util.assertNear(self, fom, monitor_fom)
            if phi >= 0.5:
                self.assertLess(energy, monitor_energy)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
