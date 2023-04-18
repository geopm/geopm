#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import os
import sys
import datetime
import io
import math
import shlex
import getpass

import geopmpy.launcher
from integration.test import util as test_util
from integration.experiment import util


# TODO: update tests to use util
def detect_launcher():
    return util.detect_launcher()


def allocation_node_test(test_exec, stdout, stderr):
    util.allocation_node_test(test_exec, stdout, stderr)


def geopmwrite(write_str):
    return util.geopmwrite(write_str)


def geopmread(read_str):
    return util.geopmread(read_str)


def get_platform():
    test_exec = "dummy -- cat /proc/cpuinfo"
    ostream = io.StringIO()
    dev_null = open('/dev/null', 'w')
    allocation_node_test(test_exec, ostream, dev_null)
    dev_null.close()
    output = ostream.getvalue()

    for line in output.splitlines():
        if line.startswith('cpu family\t:'):
            fam = int(line.split(':')[1])
        if line.startswith('model\t\t:'):
            mod = int(line.split(':')[1])
    return fam, mod


class TestLauncher(object):
    def __init__(self, app_conf, agent_conf, report_path=None,
                 trace_path=None, host_file=None, time_limit=600,
                 performance=False, fatal_test=False,
                 trace_profile_path=None, report_signals=None, trace_signals=None,
                 init_control_path=None):
        self._app_conf = app_conf
        self._agent_conf = agent_conf
        self._report_path = report_path
        self._trace_path = trace_path
        self._host_file = host_file
        self._time_limit = time_limit
        self._performance = performance
        self._trace_profile_path = trace_profile_path
        self._report_signals = report_signals
        self._trace_signals = trace_signals
        self._node_list = None
        self._exclude_list = None
        if os.environ.get('GEOPMBENCH_NO_MPI') is not None:
            self._pmpi_ctl = 'application'
        else:
            self._pmpi_ctl = 'process'
        self._job_name = 'geopm_int_test'
        self._timeout = 30
        self._disable_ompt = False
        self.set_num_cpu()
        self.set_num_rank(16)
        self.set_num_node(4)
        self._msr_save_path = None
        if fatal_test:
            self.msr_save()
        self._init_control_path = init_control_path

    def set_node_list(self, node_list):
        self._node_list = node_list

    def set_exclude_list(self, exclude_list):
        self._exclude_list = exclude_list

    def set_num_node(self, num_node):
        self._num_node = num_node
        self.set_cpu_per_rank()

    def set_num_rank(self, num_rank):
        self._num_rank = num_rank
        self.set_cpu_per_rank()

    def set_pmpi_ctl(self, pmpi_ctl):
        self._pmpi_ctl = pmpi_ctl

    def check_run(self, test_name):
        with open(test_name + '.log', 'a') as outfile:
            argv = ['dummy', detect_launcher(), '--geopm-ctl-disable', 'true']
            launcher = geopmpy.launcher.Factory().create(argv, self._num_rank, self._num_node,
                                                         self._cpu_per_rank, self._timeout,
                                                         self._time_limit, self._job_name,
                                                         self._node_list, self._exclude_list,
                                                         self._host_file)
            launcher.run(stdout=outfile, stderr=outfile)

    def run(self, test_name, include_geopm_policy=True, add_geopm_args=[]):
        """ Run the test as configured at construction time.

        Arguments:
        test_name (str):  Name of the test run to use for log files and the policy name in reports.
        include_geopm_policy (bool):  If True (default), provide --geopm-policy for the agent
        """
        if not test_util.do_launch():
            return

        self._app_conf.write()
        self._agent_conf.write()
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(str(datetime.datetime.now()) + '\n')
            outfile.flush()
            argv = ['dummy', detect_launcher(), '--geopm-ctl', self._pmpi_ctl,
                                                '--geopm-agent', self._agent_conf.get_agent(),
                                                '--geopm-profile', test_name]

            if include_geopm_policy:
                argv.extend(['--geopm-policy', self._agent_conf.get_path()])

            if self._report_path is not None:
                argv.extend(['--geopm-report', self._report_path])
            if self._trace_path is not None:
                argv.extend(['--geopm-trace', self._trace_path])
            if self._disable_ompt:
                argv.append('--geopm-ompt-disable')
            if self._trace_profile_path:
                argv.extend(['--geopm-trace-profile', self._trace_profile_path])
            if self._report_signals:
                argv.extend(['--geopm-report-signals', self._report_signals])
            if self._trace_signals:
                argv.extend(['--geopm-trace-signals', self._trace_signals])
            if self._init_control_path:
                argv.extend(['--geopm-init-control', self._init_control_path])
            argv.extend(add_geopm_args)
            argv.extend(['--'])
            exec_wrapper = os.getenv('GEOPM_EXEC_WRAPPER', '')
            if exec_wrapper:
                argv.extend(shlex.split(exec_wrapper))
            # Use app config to get path and arguments
            argv.append(self._app_conf.get_exec_path())
            argv.extend(self._app_conf.get_exec_args())
            launcher = geopmpy.launcher.Factory().create(argv, self._num_rank, self._num_node, self._cpu_per_rank, self._timeout,
                                                         self._time_limit, test_name, self._node_list, self._exclude_list, self._host_file)

            try:
                launcher.run(stdout=outfile, stderr=outfile)
            except (AttributeError, TypeError):
                if self._msr_save_path is not None:
                    self.msr_restore()
                raise

    def get_report(self, profile):
        return Report(self._report_path)

    def get_trace(self):
        return Trace(self._trace_path)

    @staticmethod
    def get_idle_nodes():
        argv = ['dummy', detect_launcher(), '--geopm-ctl-disable', 'true']
        launcher = geopmpy.launcher.Factory().create(argv, 1, 1)
        return launcher.get_idle_nodes()

    @staticmethod
    def get_alloc_nodes():
        argv = ['dummy', detect_launcher(), '--geopm-ctl-disable', 'true']
        launcher = geopmpy.launcher.Factory().create(argv, 1, 1)
        return launcher.get_alloc_nodes()

    def write_log(self, test_name, message):
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(message + '\n\n')

    def set_num_cpu(self):
        # Figure out the number of CPUs per rank leaving one for the
        # OS and one (potentially, may/may not be use depending on pmpi_ctl)
        # for the controller.
        argv = ['dummy', detect_launcher(), '--geopm-ctl-disable', 'lscpu']
        launcher = geopmpy.launcher.Factory().create(argv, 1, 1)
        ostream = io.StringIO()
        launcher.run(stdout=ostream)
        out = ostream.getvalue()
        cpu_thread_core_socket = [int(line.split(':')[1])
                       for line in out.splitlines()
                       if line.find('CPU(s):') == 0 or
                          line.find('Thread(s) per core:') == 0 or
                          line.find('Core(s) per socket:') == 0 or
                          line.find('Socket(s):') == 0]
        if self._performance == True:
            # Multiply num core per socket by num sockets, subtract 1, then multiply by threads per core.
            # Remove one CPU for BSP to calculate number of CPU for application.
            # Use hyper-threads.
            self._num_cpu = ((cpu_thread_core_socket[2] * cpu_thread_core_socket[3]) - 1) * cpu_thread_core_socket[1]
        else:
            # Multiply num core per socket by num socket and remove one
            # CPU for BSP to calculate number of CPU for application.
            # Don't use hyper-threads.
            self._num_cpu = cpu_thread_core_socket[2] * cpu_thread_core_socket[3] - 1

    def set_cpu_per_rank(self, cpu_per_rank=None):
        if cpu_per_rank is not None:
            self._cpu_per_rank = cpu_per_rank
        else:
            try:
                rank_per_node = int(math.ceil(self._num_rank / self._num_node))
                self._cpu_per_rank = int(self._num_cpu // rank_per_node)
            except (AttributeError, TypeError):
                pass

    def msr_save(self):
        """
        Snapshots all allowlisted MSRs using msrsave on all compute nodes
        that the job will be launched on.
        """
        self._msr_save_path = '/tmp/geopm-msr-save-' + getpass.getuser()
        launch_command = 'msrsave ' + self._msr_save_path
        argv = shlex.split('dummy {} --geopm-ctl-disable -- {}'
                           .format(detect_launcher(), launch_command))
        # We want to execute on every node so
        # (argv, self._num_node, self._num_node, ...
        # is intentional here and is the best we can do
        # without a allowlist of node names
        launcher = geopmpy.launcher.Factory().create(argv, self._num_node, self._num_node, self._cpu_per_rank, self._timeout,
                                                     self._time_limit, 'msr_save', self._node_list, self._host_file)
        launcher.run()

    def msr_restore(self):
        """
        Restores all allowlisted MSRs using msrsave on all compute nodes
        that the job was launched on.
        """
        if self._msr_save_path is not None:
            launch_command = 'msrsave -r ' + self._msr_save_path
            argv = shlex.split('dummy {} --geopm-ctl-disable -- {}'
                               .format(detect_launcher(), launch_command))
            # We want to execute on every node so
            # (argv, self._num_node, self._num_node, ...
            # is intentional here and is the best we can do
            # without a allowlist of node names
            launcher = geopmpy.launcher.Factory().create(argv, self._num_node, self._num_node, self._cpu_per_rank, self._timeout,
                                                         self._time_limit, 'msr_save', self._node_list, self._exclude_list, self._host_file)
            launcher.run()

    def disable_ompt(self):
        self._disable_ompt = True
