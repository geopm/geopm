#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import argparse
import os

import geopmpy.agent
import geopmpy.hash
from experiment import util
from experiment import launch_util
from experiment import machine
from experiment.monitor import monitor
from experiment.frequency_sweep import frequency_sweep


def launch_configs(output_dir, app_conf_ref, app_conf, default_freq, sweep_freqs, barrier_hash):

    # baseline run
    result = [launch_util.LaunchConfig(app_conf=app_conf_ref,
                                       agent_conf=None,
                                       name='reference')]

    # alternative baseline

    # TODO: may not always be correct
    max_uncore = float(util.geopmread('MSR::UNCORE_RATIO_LIMIT:MAX_RATIO board 0'))

    options = {'FREQ_DEFAULT': default_freq,
               'FREQ_UNCORE': max_uncore}
    config_file = os.path.join(output_dir, '{}.config'.format('fma_fixed'))
    agent_conf = geopmpy.agent.AgentConf(config_file,
                                         agent='frequency_map',
                                         options=options)
    result.append(launch_util.LaunchConfig(app_conf=app_conf_ref,
                                           agent_conf=agent_conf,
                                           name='fixed_uncore'))

    # freq map runs
    for freq in sweep_freqs:
        rid = 'fma_{:.1e}'.format(freq)
        options = {'FREQ_DEFAULT': default_freq,  # or use max or sticker from mach
                   'FREQ_UNCORE': max_uncore,
                   'HASH_0': barrier_hash,
                   'FREQ_0': freq}
        config_file = os.path.join(output_dir, '{}.config'.format(rid))
        agent_conf = geopmpy.agent.AgentConf(config_file,
                                             agent='frequency_map',
                                             options=options)
        result.append(launch_util.LaunchConfig(app_conf=app_conf,
                                               agent_conf=agent_conf,
                                               name=rid))

    return result


def report_signals():
    return monitor.report_signals()


def trace_signals():
    return ["MSR::UNCORE_PERF_STATUS:FREQ@package"]


def launch(app_conf_ref, app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    mach = machine.init_output_dir(output_dir)
    freq_range = frequency_sweep.setup_frequency_bounds(mach,
                                                        args.min_frequency,
                                                        args.max_frequency,
                                                        args.step_frequency,
                                                        add_turbo_step=True)
    barrier_hash = geopmpy.hash.crc32_str('MPI_Barrier')
    default_freq = max(freq_range)
    targets = launch_configs(output_dir=output_dir,
                             app_conf_ref=app_conf_ref,
                             app_conf=app_conf,
                             default_freq=default_freq,
                             sweep_freqs=freq_range,
                             barrier_hash=barrier_hash)

    extra_cli_args = list(experiment_cli_args)
    extra_cli_args += launch_util.geopm_signal_args(report_signals=report_signals(),
                                                    trace_signals=trace_signals())
    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=output_dir,
                                cool_off_time=args.cool_off_time,
                                enable_traces=args.enable_traces,
                                enable_profile_traces=args.enable_profile_traces)


def main(app_conf_ref, app_conf, **defaults):
    parser = argparse.ArgumentParser()
    # Use frequency sweep's run args
    frequency_sweep.setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf_ref=app_conf_ref,
           app_conf=app_conf,
           args=args,
           experiment_cli_args=extra_cli_args)
