#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running power sweep experiments.
'''

import sys
import argparse

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine
from experiment.frequency_sweep import frequency_sweep


def setup_run_args(parser):
    common_args.setup_run_args(parser)
    common_args.add_run_max_turbo(parser)
    common_args.add_min_frequency(parser)
    common_args.add_max_frequency(parser)
    common_args.add_step_frequency(parser)
    parser.add_argument('--max-uncore-frequency', dest='max_uncore_frequency',
                        action='store', type=float, default=None,
                        help='top uncore frequency setting for the sweep')
    parser.add_argument('--min-uncore-frequency', dest='min_uncore_frequency',
                        action='store', type=float, default=None,
                        help='bottom uncore frequency setting for the sweep')
    parser.add_argument('--step-uncore-frequency', dest='step_uncore_frequency',
                        action='store', type=float, default=None,
                        help='increment in hertz between uncore frequency settings for the sweep')


def setup_uncore_frequency_bounds(mach, min_uncore_freq, max_uncore_freq,
                                  step_uncore_freq):
    sys_min = 1.2e9
    sys_max = 2.7e9
    sys_step = mach.frequency_step()
    if min_uncore_freq is None:
        min_uncore_freq = sys_min
    if max_uncore_freq is None:
        max_uncore_freq = sys_max
    if step_uncore_freq is None:
        step_uncore_freq = sys_step
    if step_uncore_freq < sys_step or step_uncore_freq % sys_step != 0:
        sys.stderr.write('<geopm> Warning: uncore frequency step size may be incompatible with p-states.\n')
    if (max_uncore_freq - min_uncore_freq) % step_uncore_freq != 0:
        sys.stderr.write('<geopm> Warning: uncore frequency range not evenly divisible by step size.\n')
    if min_uncore_freq < sys_min or max_uncore_freq > sys_max:
        raise RuntimeError('Uncore frequency bounds are out of range for this system')
    if min_uncore_freq > max_uncore_freq:
        raise RuntimeError('Uncore frequency min is greater than max')

    num_step = 1 + int((max_uncore_freq - min_uncore_freq) // step_uncore_freq)
    uncore_freqs = [step_uncore_freq * ss + min_uncore_freq for ss in range(num_step)]
    uncore_freqs = sorted(uncore_freqs, reverse=True)
    return uncore_freqs


def report_signals():
    return ["CPU_CYCLES_THREAD@package", "CPU_CYCLES_REFERENCE@package",
            "TIME@package", "CPU_ENERGY@package"]


def trace_signals():
    return ['MSR::UNCORE_PERF_STATUS:FREQ@package', 'MSR::UNCORE_RATIO_LIMIT:MAX_RATIO@package',
            'MSR::UNCORE_RATIO_LIMIT:MIN_RATIO@package']


def launch_configs(app_conf, core_freq_range, uncore_freq_range):
    agent = 'frequency_map'
    targets = []
    for freq in core_freq_range:
        for uncore_freq in uncore_freq_range:
            name = '{:.1e}c_{:.1e}u'.format(freq, uncore_freq)
            options = {'FREQ_DEFAULT': freq,
                       'FREQ_UNCORE': uncore_freq}
            agent_conf = geopmpy.agent.AgentConf('{}_agent_{}c_{}u.config'.format(agent, freq, uncore_freq), agent, options)
            targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                    agent_conf=agent_conf,
                                                    name=name))
    return targets


def launch(app_conf, args, experiment_cli_args):
    '''
    Run the application over a range of core and uncore frequencies.
    '''
    mach = machine.init_output_dir(args.output_dir)
    core_freq_range = frequency_sweep.setup_frequency_bounds(mach,
                                                             args.min_frequency,
                                                             args.max_frequency,
                                                             args.step_frequency,
                                                             args.run_max_turbo)
    uncore_freq_range = setup_uncore_frequency_bounds(mach,
                                                      args.min_uncore_frequency,
                                                      args.max_uncore_frequency,
                                                      args.step_uncore_frequency)
    targets = launch_configs(app_conf, core_freq_range, uncore_freq_range)
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
                                enable_profile_traces=args.enable_profile_traces)


def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
