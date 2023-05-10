#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Helper functions useful for both tests and experiments

import os
import time
import sys
import subprocess
import timeit
import datetime

import geopmpy.launcher
from . import util
from . import machine
from apps import apps


def launch_run(agent_conf, app_conf, run_id, output_dir, extra_cli_args,
               num_nodes, enable_traces, enable_profile_traces, init_control_path):
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
    report_path = os.path.join(output_dir, '{}.report'.format(uid))
    trace_path = os.path.join(output_dir, '{}.trace'.format(uid))
    profile_trace_path = os.path.join(output_dir, '{}.ptrace'.format(uid))
    profile_name = uid
    log_path = os.path.join(output_dir, '{}.log'.format(uid))
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
    if init_control_path is not None:
        extra_cli_args += ['--geopm-init-control', init_control_path]

    argv.extend(extra_cli_args)

    # extra geopm args needed by app
    argv.extend(app_conf.get_custom_geopm_args())

    argv.extend(['--'])

    bash_path = apps.make_bash(app_conf, run_id, log_path)
    argv.extend([bash_path])

    num_ranks = app_conf.get_rank_per_node() * num_nodes
    cpu_per_rank = app_conf.get_cpu_per_rank()

    # Attempt to run if there are no valid report files for the current configuration
    if not (os.path.isfile(report_path) and os.path.getsize(report_path) > 0):

        launcher = geopmpy.launcher.Factory().create(argv,
                                                     num_node=num_nodes,
                                                     num_rank=num_ranks,
                                                     cpu_per_rank=cpu_per_rank)
        start_time = timeit.default_timer()
        launcher.run()
        end_time = timeit.default_timer()

        # Get app-reported figure of merit
        fom = app_conf.parse_fom(log_path)
        total_runtime = datetime.timedelta(seconds=end_time-start_time).total_seconds()
        # Append to report
        with open(report_path, 'a') as report:
            report.write('\nFigure of Merit: {}\n'.format(fom))
            report.write('Total Runtime: {}\n'.format(total_runtime))

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


def launch_all_runs(targets, num_nodes, iterations, extra_cli_args, output_dir,
                    cool_off_time=60, enable_traces=False, enable_profile_traces=False,
                    init_control_path=None):
    '''
    targets: a list of LaunchConfig
    iteration: integer number of iterations
    '''
    if len(targets) == 0:
        raise RuntimeError('Called launch_util.launch_all_runs() with empty target list')

    output_dir = os.path.abspath(output_dir)
    machine.init_output_dir(output_dir)
    for iteration in range(iterations):
        for tar in targets:
            agent_conf = tar.agent_conf()
            app_conf = tar.app_conf()
            run_id = tar.run_id(iteration)

            trial_complete = False
            while trial_complete is False:
                app_conf.trial_setup(run_id, output_dir)
                try:
                    launch_run(agent_conf, app_conf, run_id, output_dir,
                               extra_cli_args=extra_cli_args,
                               num_nodes=num_nodes, enable_traces=enable_traces,
                               enable_profile_traces=enable_profile_traces,
                               init_control_path=init_control_path)
                    trial_complete = True
                except subprocess.CalledProcessError as e:
                    # Hit if e.g. the app calls MPI_ABORT
                    sys.stderr.write('Warning: <geopm> Exception encountered during run {}'.format(e))
                    sys.stderr.write('Retrying previous trial...')
                    # Without sleep, the failed run tries to start over and over again and becomes
                    # unresponsive to CTRL-C.
                    time.sleep(5)
                finally:
                    app_conf.trial_teardown(run_id, output_dir)

            # rest to cool off between runs
            time.sleep(cool_off_time)

    for tar in targets:
        tar.app_conf().experiment_teardown(output_dir)

    sys.stdout.write('Run complete!\n')


def geopm_signal_args(report_signals, trace_signals):
    result = []
    if report_signals is not None:
        result += ['--geopm-report-signals=' + ','.join(report_signals)]
    if trace_signals is not None:
        result += ['--geopm-trace-signals=' + ','.join(trace_signals)]
    return result
