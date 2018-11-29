#
#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

import os
import sys
import socket
import subprocess
import datetime
import signal
import StringIO
import math

import geopm_context
import geopmpy.launcher


def detect_launcher():
    """
    Heuristic to determine the resource manager used on the system.
    Returns name of resource manager or launcher, otherwise a
    LookupError is raised.
    """
    slurm_hosts = ['mr-fusion']
    alps_hosts = ['theta']

    result = None
    hostname = socket.gethostname()

    if any(hostname.startswith(word) for word in slurm_hosts):
        result = 'srun'
    elif any(hostname.startswith(word) for word in alps_hosts):
        result = 'aprun'
    else:
        try:
            exec_str = 'srun --version'
            subprocess.check_call(exec_str, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, shell=True)
            result = 'srun'
        except subprocess.CalledProcessError:
            try:
                exec_str = 'aprun --version'
                subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, shell=True)
                result = 'aprun'
            except subprocess.CalledProcessError:
                raise LookupError('Unable to determine resource manager')
    return result


class TestLauncher(object):
    def __init__(self, app_conf, agent_conf, report_path=None,
                 trace_path=None, host_file=None, time_limit=600, region_barrier=False, performance=False):
        self._app_conf = app_conf
        self._agent_conf = agent_conf
        self._report_path = report_path
        self._trace_path = trace_path
        self._host_file = host_file
        self._time_limit = time_limit
        self._region_barrier = region_barrier
        self._performance = performance
        self._node_list = None
        self._pmpi_ctl = 'process'
        self._job_name = 'geopm_int_test'
        self._timeout = 30
        self.set_num_cpu()
        self.set_num_rank(16)
        self.set_num_node(4)

    def set_node_list(self, node_list):
        self._node_list = node_list

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
            argv = ['dummy', detect_launcher(), 'true']
            launcher = geopmpy.launcher.factory(argv, self._num_rank, self._num_node,
                                                self._cpu_per_rank, self._timeout,
                                                self._time_limit, self._job_name,
                                                self._node_list, self._host_file)
            launcher.run(stdout=outfile, stderr=outfile)

    def run(self, test_name):
        self._app_conf.write()
        self._agent_conf.write()
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(str(datetime.datetime.now()) + '\n')
            outfile.flush()
            source_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            # Using libtool causes sporadic issues with the Intel toolchain.
            exec_path = os.path.join(source_dir, '.libs', 'geopmbench')
            argv = ['dummy', detect_launcher(), '--geopm-ctl', self._pmpi_ctl,
                                                 '--geopm-agent', self._agent_conf.get_agent(),
                                                 '--geopm-policy', self._agent_conf.get_path(),
                                                 '--geopm-profile', test_name]
            if self._report_path is not None:
                argv.extend(['--geopm-report', self._report_path])
            if self._trace_path is not None:
                argv.extend(['--geopm-trace', self._trace_path])
            if self._region_barrier:
                argv.append('--geopm-barrier')
            argv.extend(['--', exec_path, '--verbose', self._app_conf.get_path()])
            launcher = geopmpy.launcher.factory(argv, self._num_rank, self._num_node, self._cpu_per_rank, self._timeout,
                                                self._time_limit, test_name, self._node_list, self._host_file)
            launcher.run(stdout=outfile, stderr=outfile)

    def get_report(self):
        return Report(self._report_path)

    def get_trace(self):
        return Trace(self._trace_path)

    @staticmethod
    def get_idle_nodes():
        argv = ['dummy', detect_launcher(), 'true']
        launcher = geopmpy.launcher.factory(argv, 1, 1)
        return launcher.get_idle_nodes()

    @staticmethod
    def get_alloc_nodes():
        argv = ['dummy', detect_launcher(), 'true']
        launcher = geopmpy.launcher.factory(argv, 1, 1)
        return launcher.get_alloc_nodes()

    def write_log(self, test_name, message):
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(message + '\n\n')

    def set_num_cpu(self):
        # Figure out the number of CPUs per rank leaving one for the
        # OS and one (potentially, may/may not be use depending on pmpi_ctl)
        # for the controller.
        argv = ['dummy', detect_launcher(), 'lscpu']
        launcher = geopmpy.launcher.factory(argv, 1, 1)
        ostream = StringIO.StringIO()
        launcher.run(stdout=ostream)
        out = ostream.getvalue()
        cpu_thread_core_socket = [int(line.split(':')[1])
                       for line in out.splitlines()
                       if line.find('CPU(s):') == 0 or
                          line.find('Thread(s) per core:') == 0 or
                          line.find('Core(s) per socket:') == 0 or
                          line.find('Socket(s):') == 0]
        if self._performance == True:
            # Mulitply num core per socket by num sockets, subtract 1, then multiply by threads per core.
            # Remove one CPU for BSP to calculate number of CPU for application.
            # Use hyper-threads.
            self._num_cpu = ((cpu_thread_core_socket[2] * cpu_thread_core_socket[3]) - 1) * cpu_thread_core_socket[1]
        else:
            # Mulitply num core per socket by num socket and remove one
            # CPU for BSP to calculate number of CPU for application.
            # Don't use hyper-threads.
            self._num_cpu = cpu_thread_core_socket[2] * cpu_thread_core_socket[3] - 1

    def set_cpu_per_rank(self):
        try:
            rank_per_node = int(math.ceil(float(self._num_rank) / float(self._num_node)))
            self._cpu_per_rank = int(math.floor(self._num_cpu / rank_per_node))
        except (AttributeError, TypeError):
            pass
