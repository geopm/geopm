#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the cpu_activity agent can improve
the energy efficiency of an application.
"""

import json
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
from experiment.uncore_frequency_sweep import gen_cpu_activity_constconfig_recommendation
from apps.arithmetic_intensity import arithmetic_intensity
from apps.minife import minife

@util.skip_unless_config_enable('beta')
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

        # Enable QM to measure total memory bandwidth

        # Assign all cores to resource monitoring association ID 0. This
        # allows for monitoring the resource usage of all cores.
        geopm_test_launcher.geopmwrite("MSR::PQR_ASSOC:RMID board 0 {}".format(0))
        # Assign the resource monitoring ID for QM Events to match the per
        # core resource association ID above (0)
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:RMID board 0 {}".format(0))
        # Select monitoring event ID 0x2 - Total Memory Bandwidth Monitoring.
        # This is used to determine the Xeon Uncore utilization.
        geopm_test_launcher.geopmwrite("MSR::QM_EVTSEL:EVENT_ID board 0 {}".format(2))

        # Grabbing system frequency parameters for experiment frequency bounds
        cpu_base_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_STICKER board 0")
        cpu_max_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MAX_AVAIL board 0")
        uncore_max_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0")

        cpu_min_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MIN_AVAIL board 0")
        uncore_min_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0")

        node_count = 1

        mach = machine.init_output_dir('.')

        def launch_helper(experiment_type, experiment_args, app_conf, experiment_cli_args):
            output_dir = experiment_args.output_dir
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)

            experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)

        ################
        # Monitor Runs #
        ################
        geopm_test_launcher.geopmwrite("CPU_FREQUENCY_MAX_CONTROL board 0 {}".format(cpu_max_freq))
        geopm_test_launcher.geopmwrite("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 {}".format(uncore_max_freq))
        geopm_test_launcher.geopmwrite("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 {}".format(uncore_min_freq))

        # MiniFE
        cls._minife_monitor_dir = Path(os.path.join('test_cpu_activity_output', 'minife_monitor'))
        experiment_args = SimpleNamespace(
            output_dir=cls._minife_monitor_dir,
            node_count=node_count,
            trial_count=1,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )

        launch_helper(monitor, experiment_args, minife.MinifeAppConf(node_count), [])

        # Arithmetic Intensity Benchmark
        cls._aib_monitor_dir = Path(os.path.join('test_cpu_activity_output', 'aib_monitor'))

        aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<26}',
             '--benchmarks=1 16'],
            mach,
            run_type='sse',
            ranks_per_node=None,
            distribute_slow_ranks=False)

        experiment_args.output_dir = cls._aib_monitor_dir
        experiment_args.trial_count = 3

        launch_helper(monitor, experiment_args, aib_app_conf, [])

        ##################################
        # Uncore frequency sweep at base #
        ##################################
        cls._aib_uncore_freq_sweep_dir = Path(os.path.join('test_cpu_activity_output', 'uncore_frequency_sweep'))
        experiment_args.output_dir = cls._aib_uncore_freq_sweep_dir
        experiment_args.min_frequency = cpu_base_freq
        experiment_args.max_frequency = cpu_base_freq
        experiment_args.step_frequency = 2e8
        experiment_args.min_uncore_frequency = uncore_min_freq
        experiment_args.max_uncore_frequency = uncore_max_freq
        experiment_args.step_uncore_frequency = 2e8
        experiment_args.run_max_turbo = False

        report_signals="MSR::QM_CTR_SCALED_RATE@package,CPU_UNCORE_FREQUENCY_STATUS@package,MSR::CPU_SCALABILITY_RATIO@package,CPU_FREQUENCY_MAX_CONTROL@package,CPU_UNCORE_FREQUENCY_MIN_CONTROL@package,CPU_UNCORE_FREQUENCY_MAX_CONTROL@package"
        experiment_cli_args=['--geopm-report-signals={}'.format(report_signals)]

        # We're reusing the AIB app conf from above here
        launch_helper(uncore_frequency_sweep, experiment_args, aib_app_conf, experiment_cli_args)

        ##############
        # Parse data #
        ##############
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report', dir_name=cls._aib_uncore_freq_sweep_dir).get_df()
        uncore_config = gen_cpu_activity_constconfig_recommendation.get_config_from_frequency_sweep(df_frequency_sweep,
                                                                                                    ['intensity_1',
                                                                                                     'intensity_16'], mach,
                                                                                                      0, 0, False)

        uncore_efficient_freq = uncore_config['CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY']['values'][0]

        #######################################################
        # Core frequency sweep at fixed uncore_efficient_freq #
        #######################################################
        cls._aib_core_freq_sweep_dir = Path(os.path.join('test_cpu_activity_output', 'core_frequency_sweep'))
        experiment_args.output_dir = cls._aib_core_freq_sweep_dir
        experiment_args.min_frequency = cpu_min_freq
        experiment_args.max_frequency = cpu_max_freq
        experiment_args.min_uncore_frequency = uncore_efficient_freq
        experiment_args.max_uncore_frequency = uncore_efficient_freq

        launch_helper(uncore_frequency_sweep, experiment_args, aib_app_conf, experiment_cli_args)

        ##############
        # Parse data #
        ##############
        df_frequency_sweep = geopmpy.io.RawReportCollection('*report', dir_name=cls._aib_core_freq_sweep_dir).get_df()

        core_config = gen_cpu_activity_constconfig_recommendation.get_config_from_frequency_sweep(df_frequency_sweep,
                                                                                                  ['intensity_1',
                                                                                                   'intensity_16'], mach,
                                                                                                    0, 0, False)
        # The core config has the updated CPU Fe value,
        # but none of the memory bandwidth info so we
        # combine them into a full const config file
        uncore_config['CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY']['values'][0] = core_config['CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY']['values'][0]

        json_config = json.dumps(uncore_config, indent=4)
        with open("const_config_io-ca.json", "w") as outfile:
            outfile.write(json_config)

        const_path = Path('const_config_io-ca.json').resolve()
        os.environ["GEOPM_CONST_CONFIG_PATH"] = str(const_path)

        ################################
        # CPU Activity Agent phi sweep #
        ################################
        geopm_test_launcher.geopmwrite("CPU_FREQUENCY_MAX_CONTROL board 0 {}".format(cpu_max_freq))
        geopm_test_launcher.geopmwrite("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 {}".format(uncore_max_freq))
        geopm_test_launcher.geopmwrite("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 {}".format(uncore_min_freq))

        # Arithmetic Intensity Benchmark
        cls._aib_agent_dir = Path(os.path.join('test_cpu_activity_output', 'aib_cpu_activity'))

        ca_experiment_args = SimpleNamespace(
            output_dir=cls._aib_agent_dir,
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
            phi_list=[0.2, 0.5, 0.7],
        )

        launch_helper(cpu_activity, ca_experiment_args, aib_app_conf, [])

        # MiniFE
        cls._minife_agent_dir = Path(os.path.join('test_cpu_activity_output', 'minife_cpu_activity'))
        ca_experiment_args.output_dir = cls._minife_agent_dir
        ca_experiment_args.trial_count = 1

        launch_helper(cpu_activity, ca_experiment_args, minife.MinifeAppConf(node_count), [])

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
