#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the cpu characterization approach
and related scripts function to create a CPU-CA characterization file with legal values.
"""

import json
import sys
import unittest
import os
from pathlib import Path
import shutil
from integration.experiment import machine
from types import SimpleNamespace

import geopmpy.agent
import geopmpy.io

from integration.test import util
from integration.test import geopm_test_launcher
from integration.experiment.energy_efficiency import cpu_activity
from integration.experiment.monitor import monitor
from integration.experiment.uncore_frequency_sweep import uncore_frequency_sweep
from integration.experiment.uncore_frequency_sweep import gen_cpu_activity_constconfig_recommendation
from integration.apps.arithmetic_intensity import arithmetic_intensity
from integration.apps.minife import minife

@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")
class TestIntegration_cpu_characterization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute the characterizer, and set up class variables.
        """
        mach = machine.init_output_dir('.')
        cls._skip_launch = not util.do_launch()

        # Grabbing system frequency parameters for experiment frequency bounds
        cls._cpu_base_freq = mach.frequency_sticker()

        # Choosing the maximum many core frequency to remove redundant frequency sweep values
        cls._cpu_max_freq = mach.frequency_max_many_core()
        cls._cpu_min_freq = mach.frequency_min()

        # The minimum and maximum values used rely on the system controls being properly configured
        # as there is no other mechanism to confirm uncore min & max at this time.  These values are
        # guaranteed to be correct after a system reset, but could have been modified by the administrator
        cls._uncore_min_freq = mach.frequency_uncore_min()
        cls._uncore_max_freq = mach.frequency_uncore_max()

        node_count = 1

        cls._run_count = 0

        #####################
        # Setup Common Args #
        #####################
        # Define global init control config
        #   Enable QM to measure total memory bandwidth for uncore utilization
        cls._initial_control_config = f"""
        # Assign all cores to resource monitoring association ID 0
        MSR::PQR_ASSOC:RMID board 0 0
        # Assign the resource monitoring ID for QM Events to match ID 0
        MSR::QM_EVTSEL:RMID board 0 0
        # Select monitoring event ID 0x2 - Total Memory Bandwidth Monitoring
        MSR::QM_EVTSEL:EVENT_ID board 0 2
        CPU_FREQUENCY_MAX_CONTROL board 0 {cls._cpu_max_freq}
        CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 {cls._uncore_max_freq}
        CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 {cls._uncore_min_freq}
        """

        experiment_args = SimpleNamespace(
            node_count=node_count,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )

        # Arithmetic Intensity Benchmark
        # Only benchmarks 1 & 16 are used to reduce characterization time
        cls._aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<26}', # quick characterization, use 26 for slower & more accurate
             '--benchmarks=1 16'], # 1 & 16 are reasonable memory and compute bound
                                   # scenarios that more closely mimic real world apps.
                                   # 0 and 32 may be used for maximum memory and compute bound
                                   # scenarios if preferred.
            mach,
            run_type='sse', # Only the SSE workload is used for characterization
            ranks_per_node=None,
            distribute_slow_ranks=False)
        cls._minife_app_conf = minife.create_appconf(mach, experiment_args)

        # use a two step characterization process to reduce runtime
        # Uncore sweep with core = base freq first
        # then core sweep with uncore at the uncore fe

        ##################################
        # Uncore frequency sweep at base #
        ##################################
        cls._aib_uncore_freq_sweep_dir = Path(os.path.join('test_cpu_characterization_output', 'uncore_frequency_sweep'))
        experiment_args.output_dir = cls._aib_uncore_freq_sweep_dir
        experiment_args.min_frequency = cls._cpu_base_freq
        experiment_args.max_frequency = cls._cpu_base_freq
        experiment_args.step_frequency = mach.frequency_step()
        experiment_args.min_uncore_frequency = cls._uncore_min_freq
        experiment_args.max_uncore_frequency = cls._uncore_max_freq
        experiment_args.step_uncore_frequency = mach.frequency_step() # using core frequency step until GEOPM provides uncore frequency step
        experiment_args.run_max_turbo = False

        report_signals="MSR::QM_CTR_SCALED_RATE@package,CPU_UNCORE_FREQUENCY_STATUS@package,MSR::CPU_SCALABILITY_RATIO@package,CPU_FREQUENCY_MAX_CONTROL@package,CPU_UNCORE_FREQUENCY_MIN_CONTROL@package,CPU_UNCORE_FREQUENCY_MAX_CONTROL@package"
        experiment_cli_args=['--geopm-report-signals={}'.format(report_signals), '--geopm-ctl=process']

        # We're using the AIB app conf from above here
        cls.launch_helper(cls, uncore_frequency_sweep, experiment_args, cls._aib_app_conf, experiment_cli_args, cls._initial_control_config)

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
        cls._aib_core_freq_sweep_dir = Path(os.path.join('test_cpu_characterization_output', 'core_frequency_sweep'))
        experiment_args.output_dir = cls._aib_core_freq_sweep_dir
        experiment_args.min_frequency = cls._cpu_min_freq
        experiment_args.max_frequency = cls._cpu_max_freq
        experiment_args.min_uncore_frequency = uncore_efficient_freq
        experiment_args.max_uncore_frequency = uncore_efficient_freq

        cls.launch_helper(cls, uncore_frequency_sweep, experiment_args, cls._aib_app_conf, experiment_cli_args, cls._initial_control_config)

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
        cc_file_name = "const_config_io-characterization.json"
        with open(cc_file_name, "w") as outfile:
            outfile.write(json_config)

        cls._const_path = Path(cc_file_name).resolve()
        os.environ["GEOPM_CONST_CONFIG_PATH"] = str(cls._const_path)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_cpu_characterization._keep_files = True


    def launch_helper(self, experiment_type, experiment_args, app_conf, experiment_cli_args, init_control_cfg):
        if not self._skip_launch:
            init_control_path = 'init_control_{}'.format(self._run_count)
            self._run_count += 1
            with open(init_control_path, 'w') as outfile:
                outfile.write(init_control_cfg)
            experiment_args.init_control = os.path.realpath(init_control_path)

            output_dir = experiment_args.output_dir
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)

            experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)


    def test_const_config_file_exists(self):
        """
        Basic testing of file creation
        """

        if not self._const_path.exists():
            raise Exception("Error: <geopm> test_cpu_characterization.py: Creation of "
                            "ConstConfigIOGroup configuration file failed")

    @util.skip_unless_batch()
    def test_const_config_signals(self):
        """
        Basic testing of signal availability and within system ranges
        """

        # Check that we can read constconfig
        cpu_freq_efficient = geopm_test_launcher.geopmread("CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY board 0")
        uncore_freq_efficient = geopm_test_launcher.geopmread("CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY board 0")

        # Check Fce within range
        self.assertGreaterEqual(cpu_freq_efficient, self._cpu_min_freq)
        self.assertLessEqual(cpu_freq_efficient, self._cpu_max_freq)

        # Check Fue within range
        self.assertGreaterEqual(uncore_freq_efficient, self._uncore_min_freq)
        self.assertLessEqual(uncore_freq_efficient, self._uncore_max_freq)

    @util.skip_unless_config_enable('beta')
    def test_cpu_activity_aib(self):
        """
        AIB testing to make sure agent tuning based on AIB yielded sensible
        results
        """
        # Arithmetic Intensity Benchmark
        aib_monitor_dir = Path(os.path.join('test_cpu_activity_aib_output', 'aib_monitor'))

        experiment_args = SimpleNamespace(
            output_dir=aib_monitor_dir,
            node_count=1,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )
        self.launch_helper(monitor, experiment_args, self._aib_app_conf, ['--geopm-ctl=process'], self._initial_control_config)

        aib_agent_dir = Path(os.path.join('test_cpu_activity_aib_output', 'aib_cpu_activity'))
        experiment_args.output_dir = aib_agent_dir
        experiment_args.phi_list = [0.2, 0.5, 0.7]
        self.launch_helper(cpu_activity, experiment_args, self._aib_app_conf, ['--geopm-ctl=process'], self._initial_control_config)

        df_monitor = geopmpy.io.RawReportCollection('*report', dir_name=aib_monitor_dir).get_df()
        monitor_runtime_16 = df_monitor[df_monitor['region'] == 'intensity_16']['runtime (s)'].mean()
        monitor_energy_16 = df_monitor[df_monitor['region'] == 'intensity_16']['package-energy (J)'].mean()

        monitor_runtime_1 = df_monitor[df_monitor['region'] == 'intensity_1']['runtime (s)'].mean()
        monitor_energy_1 = df_monitor[df_monitor['region'] == 'intensity_1']['package-energy (J)'].mean()

        df_agent = geopmpy.io.RawReportCollection('*report', dir_name=aib_agent_dir).get_df()
        df_agent_16 = df_agent[df_agent['region'] == 'intensity_16']
        df_agent_1 = df_agent[df_agent['region'] == 'intensity_1']

        # Test FoM & Runtime impact on the compute intensive workload
        # used for tuning the agent
        for phi in set(df_agent_16['CPU_PHI']):
            runtime = df_agent_16[df_agent_16['CPU_PHI'] == phi]['runtime (s)'].mean()
            energy = df_agent_16[df_agent_16['CPU_PHI'] == phi]['package-energy (J)'].mean()
            if phi < 0.5:
                util.assertNear(self, runtime, monitor_runtime_16)
                util.assertNear(self, energy, monitor_energy_16)
            if phi >= 0.5:
                self.assertLess(energy, monitor_energy_16)
                self.assertGreaterEqual(runtime, monitor_runtime_16)

        # Test FoM & Runtime impact on the memory intensive workload
        # used for tuning the agent
        for phi in set(df_agent_1['CPU_PHI']):
            runtime = df_agent_1[df_agent_1['CPU_PHI'] == phi]['runtime (s)'].mean()
            energy = df_agent_1[df_agent_1['CPU_PHI'] == phi]['package-energy (J)'].mean()

            util.assertNear(self, runtime, monitor_runtime_1)
            self.assertLess(energy, monitor_energy_1)

    @util.skip_unless_config_enable('beta')
    @util.skip_unless_workload_exists("apps/minife/miniFE_openmp-2.0-rc3/src/miniFE.x")
    def test_cpu_activity_minife(self):
        """
        MiniFE testing to make sure agent saves energy for cpu frequency
        insensitive workloads
        """
        ################
        # Monitor Runs #
        ################

        # MiniFE
        minife_monitor_dir = Path(os.path.join('test_cpu_activity_minife_output', 'minife_monitor'))
        experiment_args = SimpleNamespace(
            output_dir=minife_monitor_dir,
            node_count=1,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False,
        )
        self.launch_helper(monitor, experiment_args, self._minife_app_conf, ['--geopm-ctl=process'], self._initial_control_config)

        minife_agent_dir = Path(os.path.join('test_cpu_activity_minife_output', 'minife_cpu_activity'))
        experiment_args.output_dir = minife_agent_dir
        experiment_args.phi_list = [0.2, 0.5, 0.7]
        self.launch_helper(cpu_activity, experiment_args, self._minife_app_conf, ['--geopm-ctl=process'], self._initial_control_config)

        monitor_fom = geopmpy.io.RawReportCollection('*report', dir_name=minife_monitor_dir).get_app_df()['FOM'].mean()
        monitor_energy = geopmpy.io.RawReportCollection('*report', dir_name=minife_monitor_dir).get_epoch_df()['package-energy (J)'].mean()

        df_agent = geopmpy.io.RawReportCollection('*report', dir_name=minife_agent_dir).get_epoch_df()
        df_agent_app = geopmpy.io.RawReportCollection('*report', dir_name=minife_agent_dir).get_app_df()

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
