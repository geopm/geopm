#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running gpu frequency sweep experiments.
'''

import argparse
import os

import geopmpy.agent

from integration.experiment import launch_util
from integration.experiment import common_args
from integration.experiment import machine
from integration.experiment.frequency_sweep import frequency_sweep
from integration.experiment.uncore_frequency_sweep import uncore_frequency_sweep
from integration.experiment.gpu_frequency_sweep import gpu_frequency_sweep

def setup_run_args(parser):
    common_args.setup_run_args(parser)
    common_args.add_run_max_turbo(parser)
    common_args.add_min_frequency(parser)
    common_args.add_max_frequency(parser)
    common_args.add_step_frequency(parser)
    common_args.add_min_uncore_frequency(parser)
    common_args.add_max_uncore_frequency(parser)
    common_args.add_step_uncore_frequency(parser)

    parser.set_defaults(agent_list='frequency_map')

def explode_freq_settings(freq_range):
    freq_settings = [{}]
    for domain in freq_range:
        new_freq_settings = []
        for frequency in freq_range[domain]:
            for setting in freq_settings:
                new_val = setting.copy()
                new_val[domain] = frequency
                new_freq_settings.append(new_val)
        freq_settings = new_freq_settings
    return freq_settings

def report_signals(domains):
    signals = []
    if 'cpu' in domains:
        signals += ["CPU_CYCLES_THREAD@package",
                        "CPU_CYCLES_REFERENCE@package",
                        "TIME@package",
                        "CPU_ENERGY@package"]
    if 'gpu' in domains:
        signals += ["GPU_CORE_FREQUENCY_STATUS@board",
                        "GPU_CORE_FREQUENCY_MIN_CONTROL@board",
                        "GPU_CORE_FREQUENCY_MAX_CONTROL@board"]
    return signals

def trace_signals(domains):
    signals = []
    if 'cpu' in domains:
        signals += ["CPU_POWER@package",
                        "DRAM_POWER@package",
                        "CPU_FREQUENCY_STATUS@package",
                        "CPU_PACKAGE_TEMPERATURE@package",
                        "MSR::UNCORE_PERF_STATUS:FREQ@package",
                        "MSR::QM_CTR_SCALED_RATE@package",
                        "CPU_INSTRUCTIONS_RETIRED@package",
                        "CPU_CYCLES_THREAD@package",
                        "CPU_ENERGY@package",
                        "MSR::APERF:ACNT@package",
                        "MSR::MPERF:MCNT@package",
                        "MSR::PPERF:PCNT@package"]

    if 'gpu' in domains:
        signal += ["GPU_CORE_FREQUENCY_STATUS@gpu",
                        "GPU_POWER@gpu",
                        "GPU_UTILIZATION@gpu",
                        "GPU_CORE_ACTIVITY@gpu",
                        "GPU_UNCORE_ACTIVITY@gpu"]
    return signals

def launch_configs(output_dir, app_conf, freq_range):
    agent = 'frequency_map'
    targets = []
    policy_names = {'cpu':'FREQ_CPU_DEFAULT',
               'cpu_uncore':'FREQ_CPU_UNCORE',
               'gpu':'FREQ_GPU_DEFAULT'}

    freq_settings = explode_freq_settings(freq_range)
    print(freq_settings)

    for setting in freq_settings:
        print(f"Setting: {setting}")
        name = []
        options = {}
        for domain, freq in setting.items():
            name.append(f"{domain}{freq:.1e}")
            options[policy_names[domain]] = freq
        name_str = "_".join(name)
        file_name = os.path.join(output_dir, f'{agent}_agent_{name_str}.config')
        agent_conf = geopmpy.agent.AgentConf(file_name, agent, options)
        targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                agent_conf=agent_conf,
                                                name=name_str))
    return targets

def launch(app_conf, args, experiment_cli_args):
    '''
    Run the application over a range of core, uncore, and gpu frequencies.
    '''

    output_dir = os.path.abspath(args.output_dir)
    mach = machine.init_output_dir(output_dir)
    freq_range = {}

    if hasattr(args, 'min_frequency') and hasattr(args, 'max_frequency'):
        if args.min_frequency != args.max_frequency:
            if not hasattr(args, 'step_frequency')
                args.step_frequency = mach.frequency_step()
            freq_range['cpu'] = frequency_sweep.setup_frequency_bounds(mach,
                                                                       args.min_frequency,
                                                                       args.max_frequency,
                                                                       args.step_frequency,
                                                                       args.run_max_turbo)

    if hasattr(args, 'min_uncore_frequency') and hasattr(args, 'max_uncore_frequency'):
        if args.min_uncore_frequency != args.max_uncore_frequency:
            if not hasattr(args, 'step_uncore_frequency'):
                args.step_uncore_frequency = mach.frequency_step()
            freq_range['cpu_uncore'] = uncore_frequency_sweep.setup_uncore_frequency_bounds(mach,
                                                              args.min_uncore_frequency,
                                                              args.max_uncore_frequency,
                                                              args.step_uncore_frequency)

    if hasattr(args, 'min_gpu_frequency') and hasattr(args, 'max_gpu_frequency'):
        if args.min_gpu_frequency != args.max_gpu_frequency and machine.num_gpu() > 0:
            if not hasattr(args, 'step_gpu_frequency'):
                args.step_gpu_frqeuency = mach.gpu_frequency_step()
            freq_range['gpu'] = gpu_frequency_sweep.setup_gpu_frequency_bounds(mach,
                                                    args.min_gpu_frequency,
                                                    args.max_gpu_frequency,
                                                    args.step_gpu_frequency)

    targets = launch_configs(output_dir, app_conf, freq_range)

    extra_cli_args = launch_util.geopm_signal_args(report_signals=report_signals(freq_range.keys()),
                                                    trace_signals=trace_signals(freq_range.keys()))
    extra_cli_args += list(experiment_cli_args)

    #Set and initialize required counters for nn training
    init_control_path = os.path.join(output_dir, 'neural_net_init.controls')
    with open(init_control_path, 'w') as outfile:
        outfile.write("MSR::PQR_ASSOC:RMID board 0 0\n"
                      "# Assigns all cores to resource monitoring association ID 0\n"
                      "# Next, assign resource monitoring ID for QM events to match\n"
                      "MSR::QM_EVTSEL:RMID board 0 0\n"
                      "# Then determine Xeon Uncore Utilization\n"
                      "MSR::QM_EVTSEL:EVENT_ID board 0 0")

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=output_dir,
                                cool_off_time=args.cool_off_time,
                                enable_traces=True,
                                init_control_path=init_control_path)

def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    gpu_frequency_sweep.setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
