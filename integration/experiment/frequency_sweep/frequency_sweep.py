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
import os
import time

import geopmpy.io

from experiment import machine
from experiment import util


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


def launch_frequency_sweep(output_dir, iterations,
                           freq_range,
                           agent_types, num_node,
                           app_conf, experiment_cli_args,
                           cool_off_time=60):
    '''
    Run the application over a range of fixed processor frequencies.
    Currently only supports the frequency map agent
    '''
    machine.init_output_dir(output_dir)

    rank_per_node = app_conf.get_rank_per_node()
    num_rank = num_node * rank_per_node

    # report extensions
    report_sig = ["CYCLES_THREAD@package", "CYCLES_REFERENCE@package",
                  "TIME@package", "ENERGY_PACKAGE@package"]

    # trace extensions
    trace_sig = []
    print(experiment_cli_args)
    for iteration in range(iterations):
        for freq in freq_range:
            for agent in agent_types:
                run_id = '{}_{}'.format(freq, iteration)

                # TODO: handle energy efficient agent ?
                options = {'FREQ_DEFAULT': freq}
                agent_conf = geopmpy.io.AgentConf(os.path.join(output_dir, '{}_agent_{}.config'.format(agent, freq)), agent, options)
                agent_conf.write()

                extra_cli_args = ['--geopm-report-signals=' + ','.join(report_sig),
                                  '--geopm-trace-signals=' + ','.join(trace_sig)]
                extra_cli_args += experiment_cli_args

                util.launch_run(agent_conf=agent_conf,
                                app_conf=app_conf,
                                run_id=run_id,
                                output_dir=output_dir,
                                extra_cli_args=extra_cli_args,
                                num_node=num_node, num_rank=num_rank)  # raw launcher factory args

                # rest to cool off between runs
                time.sleep(cool_off_time)
