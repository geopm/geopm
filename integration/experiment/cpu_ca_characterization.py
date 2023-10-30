#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import os
from pathlib import Path
from types import SimpleNamespace

import geopmpy.io

from experiment import machine
from experiment import util
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
from experiment.uncore_frequency_sweep import gen_cpu_activity_constconfig_recommendation
from apps.arithmetic_intensity import arithmetic_intensity


class CPUCACharacterization(object):
    def __init__(self, full_characterization=True, base_dir='cpu_characterization'):
        self._mach = machine.init_output_dir('.')
        self._base_dir = base_dir

        if full_characterization:
            self.get_aib_conf = self._get_aib_conf_full
            self.get_experiment_args = self._get_experiment_args_full
            self._step_frequency = self._mach.frequency_step() * 2
            self._cpu_max_freq = util.geopmread('CPU_FREQUENCY_MAX_AVAIL board 0')
        else: # quick characterization
            self.get_aib_conf = self._get_aib_conf_quick
            self.get_experiment_args = self._get_experiment_args_quick
            self._step_frequency = self._mach.frequency_step()
            # Choosing the maximum many core frequency to remove redundant frequency sweep values
            self._cpu_max_freq = util.geopmread("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7 board 0")

        # Grabbing system frequency parameters for experiment frequency bounds
        self._cpu_base_freq = util.geopmread('CPU_FREQUENCY_STICKER board 0')
        self._cpu_min_freq = util.geopmread('CPU_FREQUENCY_MIN_AVAIL board 0')
        # The minimum and maximum values used rely on the system controls being properly configured
        # as there is no other mechanism to confirm uncore min & max at this time.  These values are
        # guaranteed to be correct after a system reset, but could have been modified by the administrator
        self._uncore_max_freq = util.geopmread('CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0')
        self._uncore_min_freq = util.geopmread('CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0')

    def do_characterization(self, add_init_cfg=''):
        mem_bw_cfg = self.get_init_control_config(add_init_cfg)
        init_control_file = util.create_init_control_file('mem_bw_cfg', mem_bw_cfg)

        # Use a two step characterization process to reduce runtime
        # Uncore sweep with core = base freq first
        # then core sweep with uncore at the uncore fe

        # Uncore frequency sweep
        uncore_freq_sweep_dir = Path(self._base_dir, 'uncore_frequency_sweep')
        aib_app_conf = self.get_aib_conf()
        experiment_args = self.get_experiment_args(uncore_freq_sweep_dir)
        experiment_args.init_control = os.path.realpath(init_control_file)

        experiment_args.min_frequency = self._cpu_base_freq
        experiment_args.max_frequency = self._cpu_base_freq
        experiment_args.step_frequency = self._step_frequency
        experiment_args.min_uncore_frequency = self._uncore_min_freq
        experiment_args.max_uncore_frequency = self._uncore_max_freq
        experiment_args.step_uncore_frequency = self._step_frequency
        experiment_args.run_max_turbo = False
        report_signals = \
            "MSR::QM_CTR_SCALED_RATE@package," \
            "CPU_UNCORE_FREQUENCY_STATUS@package," \
            "MSR::CPU_SCALABILITY_RATIO@package," \
            "CPU_FREQUENCY_MAX_CONTROL@package," \
            "CPU_UNCORE_FREQUENCY_MIN_CONTROL@package," \
            "CPU_UNCORE_FREQUENCY_MAX_CONTROL@package"
        experiment_cli_args = ['--geopm-report-signals={}'.format(report_signals)]

        uncore_frequency_sweep.launch(app_conf=aib_app_conf, args=experiment_args,
                                      experiment_cli_args=experiment_cli_args)

        df_frequency_sweep = \
            geopmpy.io.RawReportCollection('*report', dir_name=uncore_freq_sweep_dir).get_df()
        full_config = \
            gen_cpu_activity_constconfig_recommendation.get_config_from_frequency_sweep(
                df_frequency_sweep,
                ['intensity_1', 'intensity_16'],
                self._mach, 0, 0, False)
        uncore_fe = full_config['CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY']['values'][0]

        # Core frequency sweep at fixed uncore efficient frequency
        core_freq_sweep_dir = Path(self._base_dir, 'core_frequency_sweep')
        experiment_args.output_dir = core_freq_sweep_dir
        experiment_args.min_frequency = self._cpu_min_freq
        experiment_args.max_frequency = self._cpu_max_freq
        experiment_args.min_uncore_frequency = uncore_fe
        experiment_args.max_uncore_frequency = uncore_fe

        uncore_frequency_sweep.launch(app_conf=aib_app_conf, args=experiment_args,
                                      experiment_cli_args=experiment_cli_args)

        df_frequency_sweep = \
            geopmpy.io.RawReportCollection('*report', dir_name=core_freq_sweep_dir).get_df()
        core_config = \
            gen_cpu_activity_constconfig_recommendation.get_config_from_frequency_sweep(
                df_frequency_sweep,
                ['intensity_1', 'intensity_16'],
                self._mach, 0, 0, False)
        full_config['CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY']['values'][0] = \
            core_config['CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY']['values'][0]

        return full_config

    def get_machine_config(self):
        return self._mach

    def get_base_dir(self):
        return self._base_dir

    def get_aib_conf(self):
        pass

    def get_experiment_args(self, output_dir):
        pass

    def get_cpu_base_freq(self):
        return self._cpu_base_freq

    def get_cpu_max_freq(self):
        return self._cpu_max_freq

    def get_uncore_max_freq(self):
        return self._uncore_max_freq

    def get_cpu_min_freq(self):
        return self._cpu_min_freq

    def get_uncore_min_freq(self):
        return self._uncore_min_freq

    def get_init_control_config(self, additional_cfg=''):
        # Define global init control config
        # Enable QM to measure total memory bandwidth for uncore utilization
        mem_bw_cfg = """
        # Assign all cores to resource monitoring association ID 0
        MSR::PQR_ASSOC:RMID board 0 0
        # Assign the resource monitoring ID for QM Events to match ID 0
        MSR::QM_EVTSEL:RMID board 0 0
        # Select monitoring event ID 0x2 - Total Memory Bandwidth Monitoring
        MSR::QM_EVTSEL:EVENT_ID board 0 2
        """
        return mem_bw_cfg + additional_cfg

    def _get_aib_conf_full(self):
        # Arithmetic Intensity Benchmark
        # Only benchmarks 1 & 16 are used to reduce characterization time
        # 1 & 16 are reasonable memory and compute bound
        # scenarios that more closely mimic real world apps.
        # 0 and 32 may be used for maximum memory and compute bound
        # scenarios if preferred.
        aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<26}',
             '--benchmarks=1 16'],
            self._mach,
            run_type='sse',
            ranks_per_node=None,
            distribute_slow_ranks=False)
        return aib_app_conf

    def _get_experiment_args_full(self, output_dir):
        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=1,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False)
        return experiment_args

    def _get_aib_conf_quick(self):
        # Arithmetic Intensity Benchmark
        # Only benchmarks 1 & 16 are used to reduce characterization time
        aib_app_conf = arithmetic_intensity.ArithmeticIntensityAppConf(
            ['--slowdown=1',
             '--base-internal-iterations=10',
             '--iterations=5',
             f'--floats={1<<21}', # quick characterization, use 26 for slower & more accurate
             '--benchmarks=1 16'],
            self._mach,
            run_type='sse',
            ranks_per_node=None,
            distribute_slow_ranks=False)
        return aib_app_conf

    def _get_experiment_args_quick(self, output_dir):
        experiment_args = SimpleNamespace(
            output_dir=output_dir,
            node_count=1,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False)
        return experiment_args
