#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running CPU activity agent experiments.
'''

import argparse
import os
from decimal import Decimal

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine

def setup_run_args(parser):
    common_args.setup_run_args(parser)
    parser.add_argument('--phi-min', dest='phi_min',
                        action='store', default='0',
                        help='The minimum phi (perf-energy bias) [0,1]')
    parser.add_argument('--phi-max', dest='phi_max',
                        action='store', default='1',
                        help='The maximum phi (perf-energy bias) [0,1]')
    parser.add_argument('--phi-step', dest='phi_step',
                        action='store', default='0.1',
                        help='The step size of phi (perf-energy bias)[0,1]')
    parser.add_argument('--cpu-nn-path', dest='cpu_nn_path',
                        action='store', default=None,
                        help='Full path for the NN')

def report_signals():
    return []

def trace_signals():
    return []

def launch_configs(output_dir, app_conf, phi_min, phi_max, phi_step, cpu_nn_path):
    phi = max(min(Decimal(phi_min), Decimal(1)),Decimal(0))
    phi_max = max(min(Decimal(phi_max), Decimal(1)),phi)
    phi_step = max(min(Decimal(phi_step), Decimal(1)), Decimal("0.1"))

    config_list=[]
    config_names=[]

    while (phi <= phi_max):
        config_list.append({"CPU_PHI": float(phi)})
        config_names.append('phi' + str(phi))
        phi += phi_step

    targets = []
    agent = 'cpu_ffnet'
    for idx,config in enumerate(config_list):
        name = config_names[idx]
        options = config;
        file_name = os.path.join(output_dir, '{}_agent_{}.config'.format(agent, name))
        agent_conf = geopmpy.agent.AgentConf(file_name, agent, options)
        targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                agent_conf=agent_conf,
                                                name=name))

    if cpu_nn_path is not None:
        os.environ['GEOPM_CPU_NN_PATH'] = cpu_nn_path

    return targets

def launch(app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    extra_cli_args = launch_util.geopm_signal_args(report_signals=report_signals(),
                                                    trace_signals=trace_signals())
    extra_cli_args += experiment_cli_args

    targets = launch_configs(output_dir, app_conf, args.phi_min, args.phi_max,
                             args.phi_step, args.cpu_nn_path)

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
    args, extra_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_args)
