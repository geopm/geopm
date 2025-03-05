#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running CPU activity agent experiments.
'''

import argparse
import os

import geopmpy.agent

from integration.experiment import launch_util
from integration.experiment import common_args
from integration.experiment import machine

def setup_run_args(parser):
    common_args.setup_run_args(parser)
    parser.add_argument('--perf-energy-bias', dest='perf_energy_bias', type=float,
                        action='store', default=0,
                        help='Perf-Energy Bias [0-1] where 0 is perf-sensitive and 1 is energy-efficient')
    parser.add_argument('--cpu-nn-path', dest='cpu_nn_path',
                        action='store', default=None,
                        help='Full path for the CPU NN')
    parser.add_argument('--gpu-nn-path', dest='gpu_nn_path',
                        action='store', default=None,
                        help='Full path for the GPU NN')
    parser.add_argument('--cpu-fmap-path', dest='cpu_freq_rec_path',
                        action='store', default=None,
                        help='Full path for the CPU Frequency Recommender Map')
    parser.add_argument('--gpu-fmap-path', dest='gpu_freq_rec_path',
                        action='store', default=None,
                        help='Full path for the GPU Frequency Recommender Map')

def report_signals():
    return []

def trace_signals():
    return []

def setup_env_paths(cpu_nn_path=None, cpu_fmap_path=None, gpu_nn_path=None, gpu_fmap_path=None):
    if not cpu_nn_path and not gpu_nn_path:
        raise RuntimeError('Must specify cpu-nn-path and/or gpu-nn-path when running ffnet experiment')

    if not cpu_nn_path:
        if os.path.exists(cpu_nn_path):
            os.environ['GEOPM_CPU_NN_PATH'] = cpu_nn_path
        else:
            raise FileNotFoundError(f'File cpu-nn-path={cpu_nn_path} does not exist.')
        if not cpu_fmap_path:
            os.environ['GEOPM_CPU_FMAP_PATH'] = cpu_fmap_path
        else:
            raise RuntimeError('Must specify cpu-fmap-path when cpu-nn-path is specified for ffnet experiment')

    if not gpu_nn_path:
        if os.path.exists(gpu_nn_path):
            os.environ['GEOPM_GPU_NN_PATH'] = gpu_nn_path
        else:
            raise FileNotFoundError(f'File gpu-nn-path={gpu_nn_path} does not exist.')
        if not gpu_fmap_path:
            os.environ['GEOPM_GPU_FMAP_PATH'] = gpu_fmap_path
        else:
            raise RuntimeError('Must specify gpu-fmap-path when gpu-nn-path is specified for ffnet experiment')

def launch_configs(output_dir, app_conf, perf_energy_bias=0):
    mach = machine.init_output_dir(output_dir)

    if perf_energy_bias > 1 or perf_energy_bias < 0:
        raise ValueError('perf-energy-bias must be between 0 and 1 for ffnet experiment.')

    agent = 'ffnet'
    targets = []
    options = {"PERF_ENERGY_BIAS": perf_energy_bias}
    name = f'{perf_energy_bias}peb'

    file_name = os.path.join(output_dir, f'{agent}_agent_{name}.config'.format(agent))
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

    setup_env_paths(args.cpu_nn_path,
                    args.cpu_freq_rec_path,
                    args.gpu_nn_path,
                    args.gpu_freq_rec_path)

    targets = launch_configs(output_dir, app_conf, args.perf_energy_bias)

    #Set and initialize required counters for nn training
    init_control_path = 'neural_net_init.controls'
    with open(init_control_path, 'w') as outfile:
        outfile.write("""MSR::PQR_ASSOC:RMID board 0 0
                      # Assigns all cores to resource monitoring association ID 0
                      # Next, assign resource monitoring ID for QM events to match
                      MSR::QM_EVTSEL:RMID board 0 0
                      # Then determine Xeon Uncore Utilization
                      MSR::QM_EVTSEL:EVENT_ID board 0 0""")

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=output_dir,
                                cool_off_time=args.cool_off_time,
                                enable_traces=args.enable_traces,
                                enable_profile_traces=args.enable_profile_traces,
                                init_control_path=init_control_path)

def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_args)
