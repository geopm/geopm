#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
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
from experiment import machine
from types import SimpleNamespace

import geopmpy.agent
import geopmpy.io

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.monitor import monitor
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
from experiment.uncore_frequency_sweep import gen_cpu_activity_constconfig_recommendation
from apps.arithmetic_intensity import arithmetic_intensity

@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")
class TestIntegration_cpu_characterization(unittest.TestCase):
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
        cls._cpu_base_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_STICKER board 0")

        # Choosing the maximum many core frequency to remove redundant frequency sweep values
        cls._cpu_max_freq = geopm_test_launcher.geopmread("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7 board 0")
        cls._cpu_min_freq = geopm_test_launcher.geopmread("CPU_FREQUENCY_MIN_AVAIL board 0")

        # The minimum and maximum values used rely on the system controls being properly configured
        # as there is no other mechanism to confirm uncore min & max at this time.  These values are
        # guaranteed to be correct after a system reset, but could have been modified by the administrator
        cls._uncore_min_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0")
        cls._uncore_max_freq = geopm_test_launcher.geopmread("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0")

        node_count = 1

        mach = machine.init_output_dir('.')

        def launch_helper(experiment_type, experiment_args, app_conf, experiment_cli_args):
            output_dir = experiment_args.output_dir
            if output_dir.exists() and output_dir.is_dir():
                shutil.rmtree(output_dir)

            experiment_type.launch(app_conf=app_conf, args=experiment_args,
                                   experiment_cli_args=experiment_cli_args)

        #####################
        # Setup Common Args #
        #####################
        geopm_test_launcher.geopmwrite("CPU_FREQUENCY_MAX_CONTROL board 0 {}".format(cls._cpu_max_freq))
        geopm_test_launcher.geopmwrite("CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 {}".format(cls._uncore_min_freq))
        geopm_test_launcher.geopmwrite("CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 {}".format(cls._uncore_max_freq))

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
        aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<21}', # quick characterization, use 26 for slower & more accurate
             '--benchmarks=1 16'], # 1 & 16 are reasonable memory and compute bound
                                   # scenarios that more closely mimic real world apps.
                                   # 0 and 32 may be used for maximum memory and compute bound
                                   # scenarios if preferred.
            mach,
            run_type='sse', # Only the SSE workload is used for characterization
            ranks_per_node=None,
            distribute_slow_ranks=False)

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
        experiment_cli_args=['--geopm-report-signals={}'.format(report_signals)]

        # We're using the AIB app conf from above here
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
        cls._aib_core_freq_sweep_dir = Path(os.path.join('test_cpu_characterization_output', 'core_frequency_sweep'))
        experiment_args.output_dir = cls._aib_core_freq_sweep_dir
        experiment_args.min_frequency = cls._cpu_min_freq
        experiment_args.max_frequency = cls._cpu_max_freq
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
        cc_file_name = "const_config_io-characterization.json"
        with open(cc_file_name, "w") as outfile:
            outfile.write(json_config)

        cls._const_path = Path(cc_file_name).resolve()
        os.environ["GEOPM_CONST_CONFIG_PATH"] = str(cls._const_path)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_cpu_activity._keep_files = True

    def test_const_config_file_exists(self):
        """
        Basic testing of file creation
        """

        if not self._const_path.exists():
            raise Exception("Error: <geopm> test_cpu_characterization.py: Creation of "
                            "ConstConfigIOGroup configuration file failed")

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

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
