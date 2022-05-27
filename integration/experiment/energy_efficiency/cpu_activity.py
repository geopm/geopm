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

def report_signals():
    return []

def trace_signals():
    return []

def launch_configs(output_dir, app_conf):
    mach = machine.init_output_dir(output_dir)
    sys_min = mach.frequency_min()
    sys_max = mach.frequency_max()
    sys_sticker = mach.frequency_sticker()
    config_list = [{"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 1.0,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.9,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.8,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.7,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.6,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.5,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.4,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.3,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.2,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.1,
                    "SAMPLE_PERIOD": 0.01},
                   {"CPU_FREQ_MAX": float(nan),
                    "CPU_FREQ_EFFICIENT": float(nan),
                    "UNCORE_FREQ_MAX": float(nan),
                    "UNCORE_FREQ_EFFICIENT": float(nan),
                    "CPU_PHI": 0.0,
                    "SAMPLE_PERIOD": 0.01},
                  ]
    config_names=['phi100',
                  'phi90',
                  'phi80',
                  'phi70',
                  'phi60',
                  'phi50',
                  'phi40',
                  'phi30',
                  'phi20',
                  'phi10',
                  'phi0']

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

    targets = launch_configs(output_dir, app_conf)

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
