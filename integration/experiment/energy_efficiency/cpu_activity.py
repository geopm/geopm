#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running cpu activity agent experiments.
'''

import argparse
import os

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine

def setup_run_args(parser):
    common_args.setup_run_args(parser)
    parser.add_argument('--cpu-frequency-efficient', dest='cpu_fe',
                        action='store', default='nan',
                        help='The efficient frequency of the cpu (a.k.a. Fce) for this experiment')
    parser.add_argument('--cpu-frequency-max', dest='cpu_fmax',
                        action='store', default='nan',
                        help='The maximum frequency of the cpu (a.k.a. Fcmax) for this experiment')
    parser.add_argument('--uncore-frequency-efficient', dest='uncore_fe',
                        action='store', default='nan',
                        help='The efficient frequency of the uncore (a.k.a. Fue) for this experiment')
    parser.add_argument('--uncore-frequency-max', dest='uncore_fmax',
                        action='store', default='nan',
                        help='The maximum frequency of the uncore (a.k.a. Fumax) for this experiment')
    parser.add_argument('--uncore-mbm-list', nargs='+', help='A list of uncore_freq & max_mem_bw values in the form uncore_freq_0 max_mem_bw_0 uncore_freq_1 ...')
    parser.add_argument('--phi-list', nargs='+', help='A space seperated list of phi values (0 to 1.0) to use')

def report_signals():
    return []

def trace_signals():
    return []

def launch_configs(output_dir, app_conf, cpu_fe, cpu_fmax, uncore_fe,
                    uncore_fmax, uncore_mbm_list, phi_list):

    mach = machine.init_output_dir(output_dir)
    sys_min = mach.frequency_min()
    sys_max = mach.frequency_max()
    sys_sticker = mach.frequency_sticker()

    uncore_mbm_policy = {}
    if uncore_mbm_list is not None:
        for idx,val in enumerate(uncore_mbm_list):
            if (idx % 2) == 0:
                uncore_mbm_policy["CPU_UNCORE_FREQ_{}".format(int(idx / 2))] = float(val)
            else:
                uncore_mbm_policy["MAX_MEMORY_BANDWIDTH_{}".format(int((idx - 1) / 2))] = float(val)

    phi_values = []
    if phi_list is not None:
        phi_values = [float(val) for val in phi_list]
    else:
        phi_values = [x/10 for x in list(range(0,11))]

    config_list = [{"CPU_FREQ_MAX" : float(cpu_fmax), "CPU_FREQ_EFFICIENT": float(cpu_fe),
                    "CPU_UNCORE_FREQ_MAX" : float(uncore_fmax), "CPU_UNCORE_FREQ_EFFICIENT" : float(uncore_fe),
                    "CPU_PHI" : float(phi)} for phi in phi_values]
    config_names = ['phi'+str(int(phi*100)) for phi in phi_values]

    for config in config_list:
        config.update(uncore_mbm_policy)

    targets = []

    agent = 'cpu_activity'
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

    targets = launch_configs(output_dir, app_conf, args.cpu_fe, args.cpu_fmax, args.uncore_fe,
                                args.uncore_fmax, args.uncore_mbm_list, args.phi_list)

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
