#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running gpu frequency sweep experiments.
'''

import sys
import argparse
import os

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine
from experiment.frequency_sweep import frequency_sweep
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
import experiment.gpu_sweep


def report_signals(do_cpu=True, do_gpu=False):
    signals = []
    if do_cpu:
        signals.append(["CPU_CYCLES_THREAD@package",
                        "CPU_CYCLES_REFERENCE@package",
                        "TIME@package",
                        "CPU_ENERGY@package"])
    if do_gpu:
        signals.append(["GPU_CORE_FREQUENCY_STATUS@board",
                        "GPU_CORE_FREQUENCY_MIN_CONTROL@board",
                        "GPU_CORE_FREQUENCY_MAX_CONTROL@board"])
    return signals

def trace_signals(do_cpu=True, do_gpu=False):
    signals = []
    if do_cpu:
        signals.append(["CPU_POWER@package",
                        "DRAM_POWER@package",
                        "CPU_FREQUENCY_STATUS@package",
                        "CPU_PACKAGE_TEMPERATURE@package",
                        "MSR::UNCORE_PERF_STATUS:FREQ@package",
                        "MSR::QM_CTR_SCALED_RATE@package",
                        "CPU_INSTRUCTIONS_RETIRED@package",
                        "CPU_CYCLES_THREAD@package",
                        "CPU_ENERGY@package",
                        "MSR::APERF:ACNT@package",
                        "MSR::MPERF:MCNT@package",
                        "MSR::PPERF:PCNT@package"])
    if do_gpu:
        signals.append(["GPU_CORE_FREQUENCY_STATUS@gpu",
                        "GPU_POWER@gpu",
                        "GPU_UTILIZATION@gpu",
                        "GPU_CORE_ACTIVITY@gpu",
                        "GPU_UNCORE_ACTIVITY@gpu"])
    return signals

def launch(app_conf, args, experiment_cli_args):
    '''
    Run the application over a range of core, uncore, and gpu frequencies.
    '''

    mach = machine.init_output_dir(args.output_dir)

    do_gpu = False
    if args.min_gpu_frequency != args.max_gpu_frequency and machine.num_gpu() > 0:
        do_gpu = True

    do_cpu = True
    if args.min_cpu_frequency == args.max_cpu_frequency:
        do_cpu = False

    core_freq_range = frequency_sweep.setup_frequency_bounds(mach,
                                                             args.min_frequency,
                                                             args.max_frequency,
                                                             args.step_frequency,
                                                             args.run_max_turbo)
    uncore_freq_range = uncore_frequency_sweep.setup_uncore_frequency_bounds(
                                                             mach,
                                                             args.min_uncore_frequency,
                                                             args.max_uncore_frequency,
                                                             args.step_uncore_frequency)
    gpu_freq_range = gpu_sweep.setup_gpu_frequency_bounds(mach,
                                                args.min_gpu_frequency,
                                                args.max_gpu_frequency,
                                                args.step_gpu_frequency)

    targets = launch_configs(args.output_dir, app_conf, core_freq_range, uncore_freq_range, gpu_freq_range)

    extra_cli_args = launch_util.geopm_signal_args(report_signals=report_signals(do_cpu, do_gpu),
                                                    trace_signals=trace_signals(do_cpu, do_gpu))
    extra_cli_args += list(experiment_cli_args)

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=args.output_dir,
                                cool_off_time=args.cool_off_time,
                                enable_traces=args.enable_traces,
                                enable_profile_traces=args.enable_profile_traces)

def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    gpu_sweep.setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
