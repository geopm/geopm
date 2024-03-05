#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
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
    parser.add_argument('--power-cap',
                        type=float,
                        help='Apply a node power cap to balancers, and add a power_governor comparison')
    parser.add_argument('--agent-list',
                        type=str,
                        nargs='+',
                        choices=['monitor', 'balancer_sst_only',
                                 'balancer_dvfs_only', 'balancer_combined'],
                        default=['monitor', 'balancer_sst_only',
                                 'balancer_dvfs_only', 'balancer_combined'],
                        help='Agent configurations to be compared')


def report_signals():
    return ["CPU_CYCLES_THREAD@package", "CPU_CYCLES_REFERENCE@package",
            "TIME@package", "CPU_ENERGY@package", "TIME@core", "TIME_HINT_NETWORK@core", "CPU_FREQUENCY_STATUS@core",
            "CPU_UNCORE_FREQUENCY_STATUS@package",
            "CPU_FREQUENCY_MAX_CONTROL@core",
            "MSR::PM_ENABLE:HWP_ENABLE@package",
            #'SST::COREPRIORITY:ASSOCIATION@core',
            "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0@package",
            "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1@package",
            "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_2@package",
            "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_3@package"]


def trace_signals():
    return ['REGION_PROGRESS@core', 'REGION_HASH@core',
            'REGION_HINT@core',
            'MSR::APERF:ACNT@core', 'MSR::MPERF:MCNT@core',
            'CPU_INSTRUCTIONS_RETIRED@core', 'CPU_FREQUENCY_MAX_CONTROL@core',
            #'SST::COREPRIORITY:ASSOCIATION@core'
            ]


def launch_configs(output_dir, app_conf, agent_types=None, power_cap=None):
    targets = []
    if agent_types is None:
        agent_types = ['monitor', 'balancer_sst_only',
                       'balancer_dvfs_only', 'balancer_combined']
    if power_cap is not None and 'monitor' in agent_types and 'power_governor' not in agent_types:
        agent_types.append('power_governor')

    for agent in agent_types:
        name = agent
        if agent == 'monitor':
            options = {}
        elif agent in ['power_governor', 'power_balancer']:
            options = dict()
            if power_cap is not None:
                options['POWER_PACKAGE_LIMIT_TOTAL'] = power_cap
        else:
            options = {
                'USE_FREQUENCY_LIMITS': 0 if agent == 'balancer_sst_only' else 1,
                'USE_SST_TF': 0 if agent == 'balancer_dvfs_only' else 1
            }
            if power_cap is not None:
                options['POWER_PACKAGE_LIMIT_TOTAL'] = power_cap
        config_file = os.path.join(output_dir, name + '_agent.config')
        agent_conf = geopmpy.agent.AgentConf(path=config_file,
                                             agent='frequency_balancer' if agent.startswith('balancer_') else agent,
                                             options=options)
        targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                agent_conf=agent_conf,
                                                name=name))
    return targets


def launch(app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    agent_types = args.agent_list
    mach = machine.init_output_dir(output_dir)
    targets = launch_configs(output_dir, app_conf, agent_types, args.power_cap)
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
