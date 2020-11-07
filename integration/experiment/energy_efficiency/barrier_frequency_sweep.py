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


import argparse
import os

import geopmpy.io
import geopmpy.hash
from experiment import util
from experiment import launch_util
from experiment import machine
from experiment.monitor import monitor
from experiment.frequency_sweep import frequency_sweep


def launch_configs(output_dir, app_conf_ref, app_conf, default_freq, sweep_freqs, barrier_hash):

    # baseline run
    result = [launch_util.LaunchConfig(app_conf=app_conf_ref,
                                       agent_conf=None,
                                       name='reference')]

    # alternative baseline

    # TODO: may not always be correct
    max_uncore = float(util.geopmread('MSR::UNCORE_RATIO_LIMIT:MAX_RATIO board 0'))

    options = {'FREQ_DEFAULT': default_freq,
               'FREQ_UNCORE': max_uncore}
    config_file = os.path.join(output_dir, '{}.config'.format('fma_fixed'))
    agent_conf = geopmpy.io.AgentConf(config_file,
                                      agent='frequency_map',
                                      options=options)
    result.append(launch_util.LaunchConfig(app_conf=app_conf_ref,
                                           agent_conf=agent_conf,
                                           name='fixed_uncore'))

    # freq map runs
    for freq in sweep_freqs:
        rid = 'fma_{:.1e}'.format(freq)
        options = {'FREQ_DEFAULT': default_freq,  # or use max or sticker from mach
                   'FREQ_UNCORE': max_uncore,
                   'HASH_0': barrier_hash,
                   'FREQ_0': freq}
        config_file = os.path.join(output_dir, '{}.config'.format(rid))
        agent_conf = geopmpy.io.AgentConf(config_file,
                                          agent='frequency_map',
                                          options=options)
        result.append(launch_util.LaunchConfig(app_conf=app_conf,
                                               agent_conf=agent_conf,
                                               name=rid))

    return result


def report_signals():
    return monitor.report_signals()


def trace_signals():
    return ["MSR::UNCORE_PERF_STATUS:FREQ@package"]


def launch(app_conf_ref, app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    mach = machine.init_output_dir(output_dir)
    freq_range = frequency_sweep.setup_frequency_bounds(mach,
                                                        args.min_frequency,
                                                        args.max_frequency,
                                                        args.step_frequency,
                                                        add_turbo_step=True)
    barrier_hash = geopmpy.hash.crc32_str('MPI_Barrier')
    default_freq = max(freq_range)
    targets = launch_configs(output_dir=output_dir,
                             app_conf_ref=app_conf_ref,
                             app_conf=app_conf,
                             default_freq=default_freq,
                             sweep_freqs=freq_range,
                             barrier_hash=barrier_hash)

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


def main(app_conf_ref, app_conf, **defaults):
    parser = argparse.ArgumentParser()
    # Use frequency sweep's run args
    frequency_sweep.setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf_ref=app_conf_ref,
           app_conf=app_conf,
           args=args,
           experiment_cli_args=extra_cli_args)
