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
Helper functions for running the monitor agent.
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
    config_list = [
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.0,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.1,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.2,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.3,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.4,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.5,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.6,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.7,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.8,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 0.9,
                      "SAMPLE_PERIOD": float("NAN")},
                     {"GPU_FREQ_MAX": float("NAN"),
                      "GPU_FREQ_EFFICIENT": float("NAN"),
                      "GPU_PHI": 1.0,
                      "SAMPLE_PERIOD": float("NAN")}
                  ]
    config_names=['phi0',
                  'phi10',
                  'phi20',
                  'phi30',
                  'phi40',
                  'phi50',
                  'phi60',
                  'phi70',
                  'phi80',
                  'phi90',
                  'phi100',
                 ]

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
