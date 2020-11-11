#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
Helper functions for running power sweep experiments.
'''

import sys
import argparse
import os

import geopmpy.io

from experiment import launch_util
from experiment import common_args
from experiment import machine


def setup_run_args(parser):
    common_args.setup_run_args(parser)
    common_args.add_min_frequency(parser)
    common_args.add_max_frequency(parser)
    common_args.add_step_frequency(parser)
    common_args.add_run_max_turbo(parser)


def setup_frequency_bounds(mach, min_freq, max_freq, step_freq, add_turbo_step):
    sys_min = mach.frequency_min()
    sys_max = mach.frequency_max()
    sys_step = mach.frequency_step()
    sys_sticker = mach.frequency_sticker()
    if min_freq is None:
        min_freq = sys_min
    if max_freq is None:
        max_freq = sys_sticker
    if step_freq is None:
        step_freq = sys_step
    if step_freq < sys_step or step_freq % sys_step != 0:
        sys.stderr.write('<geopm> Warning: frequency step size may be incompatible with p-states.\n')
    if (max_freq - min_freq) % step_freq != 0:
        sys.stderr.write('<geopm> Warning: frequency range not evenly divisible by step size.\n')
    if min_freq < sys_min or max_freq > sys_max:
        raise RuntimeError('Frequency bounds are out of range for this system')

    num_step = 1 + int((max_freq - min_freq) // step_freq)
    freqs = [step_freq * ss + min_freq for ss in range(num_step)]
    if add_turbo_step and sys_max not in freqs:
        freqs.append(sys_max)
    freqs = sorted(freqs, reverse=True)
    return freqs


def report_signals():
    return ["CYCLES_THREAD@package", "CYCLES_REFERENCE@package",
            "TIME@package", "ENERGY_PACKAGE@package"]


def trace_signals():
    return ['MSR::UNCORE_PERF_STATUS:FREQ@package', 'MSR::UNCORE_RATIO_LIMIT:MAX_RATIO@package',
            'MSR::UNCORE_RATIO_LIMIT:MIN_RATIO@package']


def launch_configs(output_dir, app_conf, freq_range):
    agent = 'frequency_map'
    targets = []
    for freq in freq_range:
        name = '{:.1e}'.format(freq)
        options = {'FREQ_DEFAULT': freq}
        file_name = os.path.join(output_dir, '{}_agent_{}.config'.format(agent, freq))
        agent_conf = geopmpy.io.AgentConf(file_name, agent, options)
        targets.append(launch_util.LaunchConfig(app_conf=app_conf,
                                                agent_conf=agent_conf,
                                                name=name))
    return targets


def launch(app_conf, args, experiment_cli_args):
    '''
    Run the application over a range of fixed processor frequencies.
    Currently only supports the frequency map agent
    '''
    output_dir = os.path.abspath(args.output_dir)
    mach = machine.init_output_dir(output_dir)
    freq_range = setup_frequency_bounds(mach,
                                        args.min_frequency,
                                        args.max_frequency,
                                        args.step_frequency,
                                        args.run_max_turbo)
    targets = launch_configs(output_dir, app_conf, freq_range)
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
