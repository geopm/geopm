#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

'''
Helper functions for running gpu frequency sweep experiments.
'''

import sys
import subprocess
import argparse
import re
import math

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine
from experiment.frequency_sweep import frequency_sweep
from experiment.uncore_frequency_sweep import uncore_frequency_sweep

def setup_run_args(parser):
    uncore_frequency_sweep.setup_run_args(parser)
    parser.add_argument('--max-gpu-frequency', dest='max_gpu_frequency',
                        action='store', type=float, default=None,
                        help='top gpu frequency setting for the sweep (will pick closest legal value)')
    parser.add_argument('--min-gpu-frequency', dest='min_gpu_frequency',
                        action='store', type=float, default=None,
                        help='bottom gpu frequency setting for the sweep (will pick closest legal value')
    parser.add_argument('--step-gpu-frequency', dest='step_gpu_frequency',
                        action='store', type=float, default=None,
                        help='increment in hertz between gpu frequency settings for the sweep')

    parser.add_argument('--sample-period', dest='sample_period',
                        action='store', type=float, default=0.05,
                        help='sample period in seconds used by geopm, default 0.05s')


def setup_gpu_frequency_bounds(mach, min_gpu_freq, max_gpu_freq, step_gpu_freq):
    gclk_results = subprocess.Popen(['nvidia-smi', '-q', '-d', 'SUPPORTED_CLOCKS', '-i', '0'],universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    gclk_results_gr = subprocess.Popen(['grep','Graphics'],universal_newlines=True,stdin=gclk_results.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    gclk_results.stdout.close()
    p = re.compile(r'\d+')
    gclks = p.findall(gclk_results_gr.communicate()[0])
    gclk_results_gr.stdout.close()
    gclks = [1e6*int(i) for i in gclks]
    gclks = sorted(set(gclks), reverse=True)
    gpu_min = gclks[-1]
    gpu_max = gclks[0]

    if min_gpu_freq is None:
        min_gpu_freq = gpu_min
    if max_gpu_freq is None:
        max_gpu_freq = gpu_max
    if step_gpu_freq is None:
        step_gpu_freq = 100000000
    if min_gpu_freq < gpu_min or max_gpu_freq > gpu_max:
        raise RuntimeError('GPU frequency bounds are out of range for this system')
    if min_gpu_freq > max_gpu_freq:
        raise RuntimeError('GPU frequency min is greater than max')

    gpu_freqs = []
    if step_gpu_freq < 10000000:
        #return all entries including min/max
        gpu_freqs = list(filter(lambda freq: freq <= max_gpu_freq and freq >= min_gpu_freq, gclks))
    else:
        val = max_gpu_freq
        while val >= min_gpu_freq:
            gpu_freqs.append(int(gclks[min(range(len(gclks)), key = lambda i: abs(gclks[i]-val))]))
            val -= step_gpu_freq

    return gpu_freqs
    
def setup_uncore_frequency_bounds(mach, min_uncore_freq, max_uncore_freq,
                                  step_uncore_freq):

    if math.isnan(min_uncore_freq) or math.isnan(max_uncore_freq):
        return [float('nan')]

    return uncore_frequency_sweep.setup_uncore_frequency_bounds(mach, min_uncore_freq, max_uncore_freq, step_uncore_freq)


def report_signals():
    return uncore_frequency_sweep.report_signals() 


def trace_signals():
    return uncore_frequency_sweep.trace_signals() + \
        ['GPU_FREQUENCY_STATUS@board_accelerator', 'GPU_POWER@board_accelerator',
         'GPU_UTILIZATION@board_accelerator', 'GPU_ENERGY@board_accelerator']


def launch_configs(app_conf, core_freq_range, uncore_freq_range, gpu_freq_range, sample_period):
    agent = 'fixed_frequency'
    targets = []
    for freq in core_freq_range:
        for uncore_freq in uncore_freq_range:
            for gpu_freq in gpu_freq_range:
                name = '{:.3e}g_{:.1e}c_{:.1e}u'.format(gpu_freq, freq, uncore_freq)
                options = {'GPU_FREQUENCY': gpu_freq,
                           'CORE_FREQUENCY': freq,
                           'UNCORE_MIN_FREQUENCY': uncore_freq,
                           'UNCORE_MAX_FREQUENCY': uncore_freq,
                           'SAMPLE_PERIOD': sample_period}
                           
                agent_conf = geopmpy.agent.AgentConf('{}_agent_{}g_{}c_{}u.config'.format(agent, gpu_freq, freq, uncore_freq), agent, options)
                targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                        agent_conf=agent_conf,
                                                        name=name))
    return targets


def launch(app_conf, args, experiment_cli_args):
    '''
    Run the application over a range of core and uncore frequencies.
    '''
    mach = machine.init_output_dir(args.output_dir)
    core_freq_range = frequency_sweep.setup_frequency_bounds(mach,
                                                             args.min_frequency,
                                                             args.max_frequency,
                                                             args.step_frequency,
                                                             args.run_max_turbo)
    uncore_freq_range = setup_uncore_frequency_bounds(mach,
                                                      args.min_uncore_frequency,
                                                      args.max_uncore_frequency,
                                                      args.step_uncore_frequency)
    gpu_freq_range = setup_gpu_frequency_bounds(mach,
                                                args.min_gpu_frequency,
                                                args.max_gpu_frequency,
                                                args.step_gpu_frequency)
    targets = launch_configs(app_conf, core_freq_range, uncore_freq_range, gpu_freq_range, args.sample_period)
    extra_cli_args = launch_util.geopm_signal_args(report_signals=report_signals(),
                                                    trace_signals=trace_signals())
    extra_cli_args += list(experiment_cli_args)

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=args.output_dir,
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
