#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import os
from pathlib import Path
from types import SimpleNamespace
import json

import geopmpy.io

from experiment import machine
from experiment import util
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
from experiment.uncore_frequency_sweep import gen_cpu_activity_constconfig_recommendation
from apps.arithmetic_intensity import arithmetic_intensity


class CPUCACharacterization(object):
    def __init__(self, full_characterization=True, base_dir='cpu_characterization',
                 config_file='const_config_io-ca.json'):
        if full_characterization:
            self.get_aib_conf = self._get_aib_conf_full
        else:
            self.get_aib_conf = self._get_aib_conf_quick

        self._mach = machine.init_output_dir('.')
        self._base_dir = base_dir
        self._const_config_file = config_file

    def do_characterization(self):
        mem_bw_cfg = self.get_init_control_config()
        init_control_file = create_init_control_file('cpu_characterization', 'mem_bw_cfg',
                                                     mem_bw_cfg)

        # Grabbing system frequency parameters for experiment frequency bounds
        cpu_base_freq = util.geopmread('CPU_FREQUENCY_STICKER board 0')
        cpu_max_freq = util.geopmread('CPU_FREQUENCY_MAX_AVAIL board 0')
        uncore_max_freq = util.geopmread('CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0')
        cpu_min_freq = util.geopmread('CPU_FREQUENCY_MIN_AVAIL board 0')
        uncore_min_freq = util.geopmread('CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0')

        # Uncore frequency sweep
        uncore_freq_sweep_dir = Path(self._base_dir, 'uncore_frequency_sweep')
        aib_app_conf, experiment_args = self.get_aib_conf(uncore_freq_sweep_dir)
        experiment_args.init_control = init_control_file

        experiment_args.min_frequency = cpu_base_freq
        experiment_args.max_frequency = cpu_base_freq
        experiment_args.step_frequency = self._mach.frequency_step() * 2
        experiment_args.min_uncore_frequency = uncore_min_freq
        experiment_args.max_uncore_frequency = uncore_max_freq
        experiment_args.step_uncore_frequency = self._mach.frequency_step() * 2
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
        experiment_args.min_frequency = cpu_min_freq
        experiment_args.max_frequency = cpu_max_freq
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

        json_config = json.dumps(full_config, indent=4)
        with open(self._const_config_file, 'w') as f:
            f.write(json_config)
        const_config_path = Path(self._const_config_file).resolve()

    def get_machine_config(self):
        pass

    def get_base_dir(self):
        pass

    def get_const_config_file(self):
        pass

    def get_aib_conf(self, aib_output_dir):
        pass

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

    def _get_aib_conf_full(self, aib_output_dir):
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

        experiment_args = SimpleNamespace(
            output_dir=aib_output_dir,
            node_count=1,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False)

        return aib_app_conf, experiment_args

    def _get_aib_conf_quick(self, aib_output_dir):
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

        experiment_args = SimpleNamespace(
            output_dir=aib_output_dir,
            node_count=1,
            trial_count=3,
            cool_off_time=3,
            enable_traces=False,
            enable_profile_traces=False,
            verbose=False)

        return aib_app_conf, experiment_args


def create_init_control_file(prefix, name, contents, suffix=1):
    file_name = ''
    created = False
    n = suffix
    while not created:
        try:
            file_name, n = _generate_unique_name(prefix, name, n)
            with open(file_name, 'x') as f:
                f.write(contents)
            created = True
        except FileExistsError:
            n += 1
    return file_name


def create_output_dir(prefix, name, suffix=1):
    dir_name = ''
    created = False
    n = suffix
    while not created:
        try:
            dir_name, n = _generate_unique_name(prefix, name, n)
            os.mkdir(dir_name)
            created = True
        except FileExistsError:
            n += 1
    return dir_name


def _generate_unique_name(prefix, name, start=1):
    n = start - 1
    exists = True
    file_name = ''
    while exists:
        n += 1
        file_name = f'{prefix}-{name}-{n}'
        exists = os.path.exists(file_name)
    return file_name, n
