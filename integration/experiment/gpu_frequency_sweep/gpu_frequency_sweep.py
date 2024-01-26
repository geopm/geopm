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

def setup_run_args(parser):
    common_args.setup_run_args(parser)
    common_args.add_min_frequency(parser)
    common_args.add_max_frequency(parser)
    common_args.add_step_frequency(parser)
    common_args.add_min_uncore_frequency(parser)
    common_args.add_max_uncore_frequency(parser)
    common_args.add_step_uncore_frequency(parser)
    common_args.add_run_max_turbo(parser)
    common_args.add_agent_list(parser)

    parser.add_argument('--max-gpu-frequency', dest='max_gpu_frequency',
                        action='store', type=float, default=None,
                        help='top gpu frequency setting for the sweep')
    parser.add_argument('--min-gpu-frequency', dest='min_gpu_frequency',
                        action='store', type=float, default=None,
                        help='bottom gpu frequency setting for the sweep')
    parser.add_argument('--step-gpu-frequency', dest='step_gpu_frequency',
                        action='store', type=float, default=None,
                        help='increment in hertz between gpu frequency settings for the sweep')

    parser.set_defaults(agent_list='frequency_map')


def setup_gpu_frequency_bounds(mach, min_gpu_freq=None, max_gpu_freq=None,
                               step_gpu_freq=None):
    sys_gpu_freq_min = mach.gpu_frequency_min()
    sys_gpu_freq_max = mach.gpu_frequency_max()
    sys_gpu_freq_step = mach.gpu_frequency_step()
    if min_gpu_freq is None:
        min_gpu_freq = sys_gpu_freq_min
    if max_gpu_freq is None:
        max_gpu_freq = sys_gpu_freq_max
    if step_gpu_freq is None:
        step_gpu_freq = sys_gpu_freq_step
    if min_gpu_freq < sys_gpu_freq_min or max_gpu_freq > sys_gpu_freq_max:
        raise RuntimeError('GPU frequency bounds are out of range for this system')
    if min_gpu_freq > max_gpu_freq:
        raise RuntimeError('GPU frequency min is greater than max')
    if (max_gpu_freq - min_gpu_freq) % step_gpu_freq != 0:
        sys.stderr.write('<geopm> Warning: GPU frequency range not evenly divisible by step size.\n')

    gpu_freqs = list(reversed(experiment_steps(min_gpu_freq, max_gpu_freq, step_gpu_freq)))
    return gpu_freqs

def experiment_steps(minv, maxv, stepv):
    rval = [minv]
    while rval[-1] + stepv <= maxv:
        rval.append(rval[-1] + stepv)
    return rval

def report_signals():
    return ["CPU_CYCLES_THREAD@package", "CPU_CYCLES_REFERENCE@package",
            "TIME@package", "CPU_ENERGY@package", "GPU_CORE_FREQUENCY_STATUS@board",
            "GPU_CORE_FREQUENCY_MIN_CONTROL@board", "GPU_CORE_FREQUENCY_MAX_CONTROL@board"]


def trace_signals():
    return [ "GPU_CORE_FREQUENCY_STATUS@gpu" ]

def launch_configs(app_conf, core_freq_range, uncore_freq_range, gpu_freq_range):
    agent = 'frequency_map'
    targets = []
    for freq in core_freq_range:
        for uncore_freq in uncore_freq_range:
            for gpu_freq in gpu_freq_range:
                name = f'{freq:.1e}c_{uncore_freq:.1e}u_{gpu_freq:.1e}g'
                options = {'FREQ_CPU_DEFAULT': freq,
                           'FREQ_CPU_UNCORE': uncore_freq,
                           'FREQ_GPU_DEFAULT': gpu_freq}
                file_name = f'{agent}_agent_{name}.config'
                agent_conf = geopmpy.agent.AgentConf(file_name, agent, options)
                targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                        agent_conf=agent_conf,
                                                        name=name))
    return targets

def launch(app_conf, args, experiment_cli_args):
    '''
    Run the application over a range of core, uncore, and gpu frequencies.
    '''

    mach = machine.init_output_dir(args.output_dir)

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
    gpu_freq_range = setup_gpu_frequency_bounds(mach,
                                                args.min_gpu_frequency,
                                                args.max_gpu_frequency,
                                                args.step_gpu_frequency)

    targets = launch_configs(app_conf, core_freq_range, uncore_freq_range, gpu_freq_range)

    extra_cli_args = launch_util.geopm_signal_args(report_signals=report_signals(),
                                                    trace_signals=trace_signals())
    extra_cli_args += list(experiment_cli_args)

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=args.output_dir,
                                cool_off_time=args.cool_off_time,
                                enable_traces=args.enable_traces,
                                enable_profile_traces=args.enable_profile_traces,
                                init_control_path=args.init_control)


def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
