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

import geopmpy.agent
import geopmpy.io

from integration.test import util as test_util
from experiment import util as exp_util
from experiment.energy_efficiency import cpu_activity
from experiment.monitor import monitor
from experiment.cpu_ca_characterization import CPUCACharacterization
from apps.minife import minife

@test_util.skip_unless_config_enable('beta')
#@unittest.skip('Disabled pending resolution of issue #3015.')
@test_util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")
@test_util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx2")
@test_util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_avx512")
@test_util.skip_unless_workload_exists("apps/minife/miniFE_openmp-2.0-rc3/src/miniFE.x")
class TestIntegration_cpu_activity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
        cls._skip_launch = not test_util.do_launch()

        base_dir = 'test_cpu_activity_output'
        cls._cpu_ca_characterization = CPUCACharacterization(base_dir=base_dir)

        def launch_helper(experiment_type, experiment_args, app_conf, experiment_cli_args,
                          add_init_cfg):
            if not cls._skip_launch:
                init_control_cfg = cls._cpu_ca_characterization.get_init_control_config(add_init_cfg)
                init_control_path = exp_util.create_init_control_file('init_control', init_control_cfg)
                experiment_args.init_control = os.path.realpath(init_control_path)
                experiment_cli_args.append('--geopm-ctl=process')

                exp_util.prep_experiment_output_dir(experiment_args.output_dir)
                experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                       experiment_cli_args=experiment_cli_args)

        # Grabbing system frequency parameters for experiment frequency bounds
        cpu_max_freq = cls._cpu_ca_characterization.get_cpu_max_freq()
        uncore_max_freq = cls._cpu_ca_characterization.get_uncore_max_freq()
        uncore_min_freq = cls._cpu_ca_characterization.get_uncore_min_freq()

        ################
        # Monitor Runs #
        ################
        freq_cfg = """
        CPU_FREQUENCY_MAX_CONTROL board 0 {}
        CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 {}
        CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 {}
        """.format(cpu_max_freq, uncore_max_freq, uncore_min_freq)

        # MiniFE
        cls._minife_monitor_dir = Path(base_dir, 'minife_monitor')
        experiment_args = cls._cpu_ca_characterization.get_experiment_args(cls._minife_monitor_dir)
        experiment_args.trial_count = 1
        launch_helper(monitor, experiment_args, minife.MinifeAppConf(experiment_args.node_count),
                      [], freq_cfg)

        # Arithmetic Intensity Benchmark
        aib_app_conf = cls._cpu_ca_characterization.get_aib_conf()
        cls._aib_monitor_dir = Path(base_dir, 'aib_monitor')
        experiment_args.output_dir = cls._aib_monitor_dir
        experiment_args.trial_count = 3
        launch_helper(monitor, experiment_args, aib_app_conf, [], freq_cfg)

        cpu_ca_config = cls._cpu_ca_characterization.do_characterization()

        json_config = json.dumps(cpu_ca_config, indent=4)
        cc_file_name = 'const_config_io-ca.json'
        with open(cc_file_name, "w") as outfile:
            outfile.write(json_config)

        const_path = Path(cc_file_name).resolve()
        os.environ['GEOPM_CONST_CONFIG_PATH'] = str(const_path)

        ################################
        # CPU Activity Agent phi sweep #
        ################################
        # Arithmetic Intensity Benchmark
        cls._aib_agent_dir = Path(base_dir, 'aib_cpu_activity')
        ca_experiment_args = cls._cpu_ca_characterization.get_experiment_args(cls._aib_agent_dir)
        ca_experiment_args.phi_list = [0.2, 0.5, 0.7]
        launch_helper(cpu_activity, ca_experiment_args, aib_app_conf, [], freq_cfg)

        # MiniFE
        cls._minife_agent_dir = Path(base_dir, 'minife_cpu_activity')
        ca_experiment_args.output_dir = cls._minife_agent_dir
        ca_experiment_args.trial_count = 1
        launch_helper(cpu_activity, ca_experiment_args,
                      minife.MinifeAppConf(ca_experiment_args.node_count), [], freq_cfg)

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
                test_util.assertNear(self, runtime, monitor_runtime_16)
                test_util.assertNear(self, energy, monitor_energy_16)
            if phi >= 0.5:
                self.assertGreaterEqual(runtime, monitor_runtime_16)

        # Test FoM & Runtime impact on the memory intensive workload
        # used for tuning the agent
        for phi in set(df_agent_1['CPU_PHI']):
            runtime = df_agent_1[df_agent_1['CPU_PHI'] == phi]['runtime (s)'].mean()
            energy = df_agent_1[df_agent_1['CPU_PHI'] == phi]['package-energy (J)'].mean()

            if phi <= 0.5:
                test_util.assertNear(self, runtime, monitor_runtime_1)
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
                test_util.assertNear(self, fom, monitor_fom)
            if phi >= 0.5:
                self.assertLess(energy, monitor_energy)

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    test_util.do_launch()
    unittest.main()
