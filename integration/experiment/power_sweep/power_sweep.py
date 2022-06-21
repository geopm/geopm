#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running power sweep experiments.
'''

import sys
import math
import argparse
import os

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine


def setup_run_args(parser):
    common_args.setup_run_args(parser)
    common_args.add_min_power(parser)
    common_args.add_max_power(parser)
    common_args.add_step_power(parser)
    common_args.add_agent_list(parser)
    parser.set_defaults(agent_list='power_governor,power_balancer',
                        enable_traces=True)


def setup_power_bounds(mach, min_power, max_power, step_power):
    if min_power is None:
        # system minimum is actually too low; use 50% of TDP or min rounded up to nearest step, whichever is larger
        min_power = max(0.5 * mach.power_package_tdp(), mach.power_package_min())
        min_power = step_power * math.ceil(float(min_power) / step_power)
        sys.stderr.write("Warning: <geopm> run_power_sweep: Invalid or unspecified min_power; using default minimum: {}.\n".format(min_power))

    if max_power is None:
        max_power = mach.power_package_tdp()
        sys.stderr.write("Warning: <geopm> run_power_sweep: Invalid or unspecified max_power; using system TDP: {}.\n".format(max_power))

    if (min_power < mach.power_package_min() or
        max_power > mach.power_package_max() or
        min_power > max_power):
        raise RuntimeError('Power bounds are out of range for this system')

    return int(min_power), int(max_power)


def report_signals():
    return ["CPU_CYCLES_THREAD@package", "CPU_CYCLES_REFERENCE@package",
            "TIME@package", "CPU_ENERGY@package"]


def trace_signals():
    return ["MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT@package",
            "TEMPERATURE_PACKAGE@package"]


def launch_configs(output_dir, app_conf, agent_types, min_power, max_power, step_power):
    """
    Runs the application under a range of socket power limits.  Used
    by other analysis types to run either the PowerGovernorAgent or
    the PowerBalancerAgent.
    """

    targets = []
    min_power = int(min_power)
    max_power = int(max_power)
    step_power = int(step_power)
    for power_cap in range(max_power, min_power-1, -step_power):
        for agent in agent_types:
            name = '{}'.format(power_cap)
            options = {'power_budget': power_cap}
            config_file = os.path.join(output_dir, name + '_agent.config')
            agent_conf = geopmpy.agent.AgentConf(path=config_file,
                                                 agent=agent,
                                                 options=options)
            targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                    agent_conf=agent_conf,
                                                    name=name))
    return targets


def launch(app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    agent_types = args.agent_list.split(',')
    mach = machine.init_output_dir(output_dir)
    min_power, max_power = setup_power_bounds(mach, args.min_power,
                                              args.max_power, args.step_power)
    targets = launch_configs(output_dir, app_conf, agent_types, min_power, max_power, args.step_power)
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


def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
