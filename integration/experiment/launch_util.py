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

# Helper functions useful for both tests and experiments

import os
import time
import sys

import geopmpy.launcher
from . import util
from . import machine
from apps import apps


def launch_run(agent_conf, app_conf, run_id, output_dir, extra_cli_args,
               num_nodes, enable_traces, enable_profile_traces):
    # launcher and app should create files in output_dir
    start_dir = os.getcwd()
    os.chdir(output_dir)

    # TODO: why does launcher strip off first arg, rather than geopmlaunch main?
    argv = ['dummy', util.detect_launcher()]

    agent_name = 'monitor'
    if agent_conf is not None:
        agent_name = agent_conf.get_agent()
        agent_conf.write()
    if agent_name != 'monitor':
        argv.append('--geopm-agent=' + agent_conf.get_agent())
        argv.append('--geopm-policy=' + agent_conf.get_path())

    app_name = app_conf.name()

    uid = '{}_{}_{}'.format(app_name.lower(), agent_name, run_id)
    report_path = '{}.report'.format(uid)
    trace_path = '{}.trace'.format(uid)
    profile_trace_path = '{}.ptrace'.format(uid)
    profile_name = uid
    log_path = '{}.log'.format(uid)
    sys.stdout.write('Run commencing...\nLive job output will be written to: {}\n'
                     .format(os.path.join(output_dir, log_path)))

    # TODO: these are not passed to launcher create()
    # some are generic enough they could be, though
    extra_cli_args += ['--geopm-report', report_path,
                       '--geopm-profile', profile_name,
    ]
    if enable_traces:
        extra_cli_args += ['--geopm-trace', trace_path]
    if enable_profile_traces:
        extra_cli_args += ['--geopm-trace-profile', profile_trace_path]

    argv.extend(extra_cli_args)

    # extra geopm args needed by app
    argv.extend(app_conf.get_custom_geopm_args())

    argv.extend(['--'])

    bash_path = apps.make_bash(app_conf, run_id, log_path)
    argv.extend([bash_path])

    num_ranks = app_conf.get_rank_per_node() * num_nodes

    launcher = geopmpy.launcher.Factory().create(argv,
                                                 num_node=num_nodes,
                                                 num_rank=num_ranks)
    launcher.run()

    # Get app-reported figure of merit
    fom = app_conf.parse_fom(log_path)
    # Append to report
    with open(report_path, 'a') as report:
        report.write('\nFigure of Merit: {}\n'.format(fom))

    # return to previous directory
    os.chdir(start_dir)


class LaunchConfig():
    def __init__(self, app_conf, agent_conf, name):
        self._app_conf = app_conf
        self._agent_conf = agent_conf
        self._name = name

    def app_conf(self):
        return self._app_conf

    def agent_conf(self):
        return self._agent_conf

    def run_id(self, iteration):
        run_id = '{}'.format(iteration)
        if self._name:
            run_id = '{}_{}'.format(self._name, run_id)
        return run_id


def launch_all_runs(targets, num_nodes, iterations, extra_cli_args, output_dir, cool_off_time=60, enable_traces=False, enable_profile_traces=False):
    '''
    targets: a list of LaunchConfig
    iteration: integer number of iterations
    '''
    machine.init_output_dir(output_dir)

    for iteration in range(iterations):
        for tar in targets:
            agent_conf = tar.agent_conf()
            app_conf = tar.app_conf()
            run_id = tar.run_id(iteration)

            launch_run(agent_conf, app_conf, run_id, output_dir,
                       extra_cli_args=extra_cli_args,
                       num_nodes=num_nodes, enable_traces=enable_traces,
                       enable_profile_traces=enable_profile_traces)

            # rest to cool off between runs
            time.sleep(cool_off_time)
    sys.stdout.write('Run complete!\n')


def geopm_signal_args(report_signals, trace_signals):
    result = []
    if report_signals is not None:
        result += ['--geopm-report-signals=' + ','.join(report_signals)]
    if trace_signals is not None:
        result += ['--geopm-trace-signals=' + ','.join(trace_signals)]
    return result
