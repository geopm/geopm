#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

import socket
import subprocess
import os
import datetime
import signal


def get_resource_manager():
    # FIXME This should do some better autodetection rather than looking at the hostname.
    hostname = socket.gethostname()
    slurm_hosts = ['mr-fusion', 'KNP12']
    if any(hostname.startswith(word) for word in slurm_hosts):
        return "SLURM"
    elif hostname.find('theta') == 0:
        return "ALPS"
    else:
        raise LookupError('Unrecognized hostname: ' + hostname)


def factory(app_conf, ctl_conf, report_path,
                     trace_path=None, host_file=None, time_limit=1):
    resource_manager = get_resource_manager()
    if resource_manager == "SLURM":
        return SrunLauncher(app_conf, ctl_conf, report_path,
                            trace_path, host_file, time_limit)
    elif resource_manager == "ALPS":
        return AlpsLauncher(app_conf, ctl_conf, report_path,
                              trace_path, host_file, time_limit)
    else:
        raise LookupError('Unrecognized hostname: ' + hostname)


class Launcher(object):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None, time_limit=None, region_barrier=False):
        self._app_conf = app_conf
        self._ctl_conf = ctl_conf
        self._report_path = report_path
        self._trace_path = trace_path
        self._host_file = host_file
        self._time_limit = time_limit
        self._region_barrier = region_barrier
        self._node_list = None
        self._pmpi_ctl = 'process'
        self._default_handler = signal.getsignal(signal.SIGINT)
        self.set_num_rank(16)
        self.set_num_node(4)

    def __repr__(self):
        output = ''
        for k,v in self._environ().iteritems():
            output += '{k}={v} '.format(k=k, v=v)
        output += self._exec_str()
        return output

    def __str__(self):
        return self.__repr__()

    def _int_handler(self, signum, frame):
        """
        This is necessary to prevent the script from dying on the first CTRL-C press.  SLURM requires 2
        SIGINT signals to abort the job.
        """
        if type(self) == SrunLauncher:
            print "srun: interrupt (one more within 1 sec to abort)"
        else:
            self._default_handler(signum, frame)

    def set_node_list(self, node_list):
        self._node_list = node_list

    def set_num_node(self, num_node):
        self._num_node = num_node
        self._set_num_thread()

    def set_num_rank(self, num_rank):
        self._num_rank = num_rank
        self._set_num_thread()

    def set_pmpi_ctl(self, pmpi_ctl):
        self._pmpi_ctl = pmpi_ctl

    def check_run(self, test_name):
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(str(datetime.datetime.now()) + '\n\n' )
            outfile.write(self._check_str() + '\n\n')
            outfile.flush()
            signal.signal(signal.SIGINT, self._int_handler)
            subprocess.check_call(self._check_str(), shell=True, stdout=outfile, stderr=outfile)
            signal.signal(signal.SIGINT, self._default_handler)

    def run(self, test_name):
        env = dict(os.environ)
        env.update(self._environ())
        self._app_conf.write()
        self._ctl_conf.write()
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(str(datetime.datetime.now()) + '\n\n' )
            outfile.write(str(self) + '\n\n')
            outfile.flush()
            signal.signal(signal.SIGINT, self._int_handler)
            subprocess.check_call(self._exec_str(), shell=True, env=env, stdout=outfile, stderr=outfile)
            signal.signal(signal.SIGINT, self._default_handler)

    def get_report(self):
        return Report(self._report_path)

    def get_trace(self):
        return Trace(self._trace_path)

    def get_idle_nodes(self):
        return ''

    def get_alloc_nodes(self):
        return ''

    def write_log(self, test_name, message):
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(message + '\n\n')

    def _set_num_thread(self):
        # Figure out the number of CPUs per rank leaving one for the
        # OS and one (potentially, may/may not be use depending on pmpi_ctl)
        # for the controller.
        cmd = ' '.join((self._exec_option(), 'lscpu'))
        signal.signal(signal.SIGINT, self._int_handler)
        pid = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out, err) = pid.communicate()
        signal.signal(signal.SIGINT, self._default_handler)
        if pid.returncode:
            raise subprocess.CalledProcessError(pid.returncode, cmd, err)
        core_socket = [int(line.split(':')[1])
                       for line in out.splitlines()
                       if line.find('Core(s) per socket:') == 0 or
                          line.find('Socket(s):') == 0]
        # Mulitply num core per socket by num socket and remove one
        # CPU for BSP and one for the controller to calculate number
        # of CPU for application.  Don't use hyper-threads.
        num_cpu = core_socket[0] * core_socket[1] - 2
        try:
            rank_per_node = self._num_rank / self._num_node
            # Fixes situations when rank_per_node may be 0 which results in a divide by 0 below.
            rank_per_node = rank_per_node if rank_per_node != 0 else 1
            self._num_thread = num_cpu / rank_per_node
        except AttributeError:
            pass

    def _environ(self):
        result = {'LD_DYNAMIC_WEAK': 'true',
                  'OMP_NUM_THREADS' : str(self._num_thread),
                  'GEOPM_PMPI_CTL' : self._pmpi_ctl,
                  'GEOPM_REPORT' : self._report_path,
                  'GEOPM_POLICY' : self._ctl_conf.get_path()}
        if self._trace_path:
            result['GEOPM_TRACE'] = self._trace_path
        if self._region_barrier:
            result['GEOPM_REGION_BARRIER'] = 'true'
        return result

    def _check_str(self):
        # Save the pmpi_ctl state since we don't need it to run 'true'
        tmp_pmpi_ctl = self._pmpi_ctl
        self._pmpi_ctl = ''
        check_str = ' '.join((self._mpiexec_option(),
                              self._num_node_option(),
                              self._num_rank_option(),
                              'true'))
        self._pmpi_ctl = tmp_pmpi_ctl
        return check_str

    def _exec_str(self):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        # Using libtool causes sporadic issues with the Intel toolchain.
        exec_path = os.path.join(script_dir, '.libs', 'geopm_test_integration --verbose')
        return ' '.join((self._mpiexec_option(),
                         self._num_node_option(),
                         self._num_rank_option(),
                         self._affinity_option(),
                         self._host_option(),
                         self._membind_option(),
                         exec_path,
                         self._app_conf.get_path()))

    def _num_rank_option(self):
        num_rank = self._num_rank
        if self._pmpi_ctl == 'process':
            num_rank += self._num_node
        return '-n {num_rank}'.format(num_rank=num_rank)

    def _affinity_option(self):
        return ''

    def _membind_option(self):
        return ''

    def _host_option(self):
        if self._host_file:
            raise NotImplementedError
        return ''


class SrunLauncher(Launcher):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None, time_limit=1):
        self._queuing_timeout = 30
        self._job_name = 'int_test'
        super(SrunLauncher, self).__init__(app_conf, ctl_conf, report_path,
                                           trace_path=trace_path, host_file=host_file,
                                           time_limit=time_limit)

    def _mpiexec_option(self):
        mpiexec = 'srun -I{timeout} -J {name}'.format(timeout=self._queuing_timeout, name=self._job_name)
        if self._time_limit is not None:
            mpiexec += ' -t {time_limit}'.format(time_limit=self._time_limit)
        if self._node_list is not None:
            mpiexec += ' -w ' + ','.join(self._node_list)
        return mpiexec

    def _exec_option(self):
        return 'srun -I{timeout} -J {name} -n 1'.format(timeout=self._queuing_timeout, name=self._job_name)

    def _num_node_option(self):
        return '-N {num_node}'.format(num_node=self._num_node)

    def _affinity_option(self):
        proc_mask = self._num_thread * '1' + '00'
        result_base = '--cpu_bind=v,mask_cpu:'
        mask_list = []
        if (self._pmpi_ctl == 'process'):
            mask_list.append('0x2')
        for ii in range(self._num_rank / self._num_node):
            mask_list.append('0x{:x}'.format(int(proc_mask, 2)))
            proc_mask = proc_mask + self._num_thread * '0'
        return result_base + ','.join(mask_list)

    def _host_option(self):
        result = ''
        if self._host_file:
            result = '-w {host_file}'.format(self._host_file)
        return result

    def get_idle_nodes(self):
        return subprocess.check_output('sinfo -t idle -hNo %N', shell=True).splitlines()

    def get_alloc_nodes(self):
        return subprocess.check_output('sinfo -t alloc -hNo %N', shell=True).splitlines()

class AlpsLauncher(Launcher):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None, time_limit=1):
        self._queuing_timeout = 30
        self._job_name = 'int_test'
        super(AlpsLauncher, self).__init__(app_conf, ctl_conf, report_path,
                                           trace_path=trace_path, host_file=host_file,
                                           time_limit=time_limit)

    def _environ(self):
        result = super(AlpsLauncher, self)._environ()
        result['KMP_AFFINITY'] = 'disabled'
        return result

    def _mpiexec_option(self):
        mpiexec = 'aprun'
        if self._time_limit is not None:
            mpiexec += ' -t {time_limit}'.format(time_limit=self._time_limit * 60)
        if self._node_list is not None:
            mpiexec += ' -L ' + ','.join(self._node_list)
        return mpiexec

    def _exec_option(self):
        return 'aprun -n 1'

    def _num_node_option(self):
        rank_per_node = self._num_rank / self._num_node
        if self._pmpi_ctl == 'process':
            rank_per_node += 1
        return '-N {rank_per_node}'.format(rank_per_node=rank_per_node)

    def _affinity_option(self):
        result_base = '-cc '
        mask_list = []
        off_start = 1
        if (self._pmpi_ctl == 'process'):
            mask_list.append('1')
            off_start = 2
        rank_per_node = self._num_rank / self._num_node
        thread_per_node = rank_per_node * self._num_thread
        mask_list.extend(['{0}-{1}'.format(off, off + self._num_thread - 1)
                          for off in range(off_start,thread_per_node + off_start, self._num_thread)])
        return result_base + ':'.join(mask_list)

    def _host_option(self):
        result = ''
        if self._host_file:
            result = '--node-list-file {host_file}'.format(self._host_file)
        return result

    def get_idle_nodes(self):
        raise NotImplementedError;

    def get_alloc_nodes(self):
        raise NotImplementedError;
