#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running gpu activity agent experiments.
'''

import argparse
import os

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine

def setup_run_args(parser):
    common_args.setup_run_args(parser)
    parser.add_argument('--gpu-frequency-efficient', dest='gpu_fe',
                        action='store', default='nan',
                        help='The efficient frequency of the gpu (a.k.a. Fe) for this experiment')
    parser.add_argument('--gpu-frequency-max', dest='gpu_fmax',
                        action='store', default='nan',
                        help='The maximum frequency of the gpu (a.k.a. Fmax) for this experiment')

def report_signals():
    return []

def trace_signals():
    return []

def launch_configs(output_dir, app_conf, gpu_fe, gpu_fmax):
    mach = machine.init_output_dir(output_dir)
    sys_min = mach.frequency_min()
    sys_max = mach.frequency_max()
    sys_sticker = mach.frequency_sticker()

    config_list = [{"GPU_FREQ_MAX" : float(gpu_fmax),
                    "GPU_FREQ_EFFICIENT": float(gpu_fe),
                    "GPU_PHI" : float(phi/10),
                    "SAMPLE_PERIOD": float('nan')} for phi in range(0,11)]
    config_names = ['phi'+str(x*10) for x in range(0,11)]

    targets = []

    agent = 'gpu_activity'
    for idx,config in enumerate(config_list):
        name = config_names[idx]
        options = config;
        file_name = os.path.join(output_dir, '{}_agent_{}.config'.format(agent, name))
        agent_conf = geopmpy.agent.AgentConf(file_name, agent, options)
        targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                agent_conf=agent_conf,
                                                name=name))
    return targets

def launch(app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    extra_cli_args = launch_util.geopm_signal_args(report_signals=report_signals(),
                                                    trace_signals=trace_signals())
    extra_cli_args += experiment_cli_args

    targets = launch_configs(output_dir, app_conf, args.gpu_fe, args.gpu_fmax)

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
