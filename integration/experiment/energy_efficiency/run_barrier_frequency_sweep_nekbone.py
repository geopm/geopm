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
Frequency map experiment comparing nekbone with added barriers run
at a lower frequency to the baseline with no added barriers.
'''

import argparse

import geopmpy.io
import geopmpy.hash
from experiment import common_args
from experiment import machine
from experiment import launch_util
from experiment.frequency_sweep import frequency_sweep
from apps.nekbone import nekbone


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_nodes(parser)
    common_args.add_min_frequency(parser)
    common_args.add_max_frequency(parser)
    common_args.add_iterations(parser)
    # TODO: option for add turbo step

    args, extra_cli_args = parser.parse_known_args()

    output_dir = args.output_dir
    num_nodes = args.nodes

    # application parameters
    baseline_app = nekbone.NekboneAppConf(add_barriers=False)
    target_app = nekbone.NekboneAppConf(add_barriers=True)
    barrier_hash = geopmpy.hash.crc32_str('MPI_Barrier')

    # experiment parameters
    mach = machine.init_output_dir(output_dir)
    min_freq = args.min_frequency
    max_freq = args.max_frequency
    step_freq = None
    freqs = frequency_sweep.setup_frequency_bounds(mach, min_freq, max_freq, step_freq,
                                                   add_turbo_step=True)
    default_freq = max(freqs)
    iterations = args.iterations

    report_sig = ["CYCLES_THREAD@package", "CYCLES_REFERENCE@package",
                  "TIME@package", "ENERGY_PACKAGE@package"]
    extra_cli_args += launch_util.geopm_signal_args(report_sig, [])

    # baseline run
    targets = [launch_util.LaunchConfig(app_conf=baseline_app, agent_conf=None, name='base')]

    # freq map runs
    for freq in freqs:
        rid = 'fma_{:.1e}'.format(freq)
        options = {'FREQ_DEFAULT': default_freq,  # or use max or sticker from mach
                   'HASH_0': barrier_hash,
                   'FREQ_0': freq}
        agent_conf = geopmpy.io.AgentConf('{}.config'.format(rid),
                                          agent='frequency_map',
                                          options=options)
        targets.append(launch_util.LaunchConfig(app_conf=target_app, agent_conf=agent_conf, name=rid))

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=num_nodes,
                                iterations=iterations,
                                extra_cli_args=extra_cli_args,
                                output_dir=output_dir)
