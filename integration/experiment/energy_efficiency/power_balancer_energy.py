#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import os

from experiment import launch_util
from experiment import common_args
from experiment import machine
from experiment.power_sweep import power_sweep
from experiment.monitor import monitor


def launch_configs(output_dir, app_conf_ref, app_conf, agent_types, min_power, max_power, step_power):
    launch_configs = [launch_util.LaunchConfig(app_conf=app_conf_ref,
                                               agent_conf=None,
                                               name="reference")]

    launch_configs += power_sweep.launch_configs(output_dir=output_dir,
                                                 app_conf=app_conf,
                                                 agent_types=agent_types,
                                                 min_power=min_power,
                                                 max_power=max_power,
                                                 step_power=step_power)
    return launch_configs


def report_signals():
    report_sig = set(monitor.report_signals())
    report_sig.update(power_sweep.report_signals())
    report_sig = sorted(list(report_sig))
    return report_sig


def trace_signals():
    return power_sweep.trace_signals()


def launch(app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    agent_types = args.agent_list.split(',')
    mach = machine.init_output_dir(output_dir)
    min_power, max_power = power_sweep.setup_power_bounds(mach, args.min_power,
                                                          args.max_power, args.step_power)
    targets = launch_configs(output_dir, app_conf, app_conf, agent_types, min_power, max_power, args.step_power)
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
                                enable_profile_traces=args.enable_profile_traces,
                                init_control_path=args.init_control)


def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    # Use power sweep's run args
    power_sweep.setup_run_args(parser)
    parser.set_defaults(**defaults)
    # Change default agent_list (power_sweep runs the governor also)
    parser.set_defaults(agent_list='power_balancer')
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
