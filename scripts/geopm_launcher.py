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
"""\
GEOPM options:
      --geopm-ctl=ctl         use geopm runtime and launch geopm with the
                              "ctl" method, one of "process", "pthread" or
                              "application"
      --geopm-policy=pol      use the geopm policy file or shared memory
                              region "pol"
      --geopm-report=path     create geopm report files with base name "path"
      --geopm-trace=path      create geopm trace files with base name "path"
      --geopm-shmkey=key      use shared memory keys for geopm starting with
                              "key"
      --geopm-timeout=sec     appliation waits "sec" seconds for handshake
                              with geopm
      --geopm-plugin=path     look for geopm plugins in "path", a : separated
                              list of directories
      --geopm-debug-attach=rk attach serial debugger to rank "rk"
      --geopm-barrier         apply node local barriers when application enters
                              or exits a geopm region
      --geopm-preload         Use LD_PRELOAD to link libgeopm.so at runtime

"""

import sys
import os
import optparse
import subprocess
import socket
import math
import signal

def resource_manager():
    slurm_hosts = ['mr-fusion']
    alps_hosts = ['theta']

    result = os.environ.get('GEOPM_RM')

    if not result:
        hostname = socket.gethostname()

        if sys.argv[0].endswith('srun'):
            result = "SLURM"
        elif sys.argv[0].endswith('aprun'):
            result = "ALPS"
        elif any(hostname.startswith(word) for word in slurm_hosts):
            result = "SLURM"
        elif any(hostname.startswith(word) for word in alps_hosts):
            result = "ALPS"
        else:
            try:
                exec_str = 'srun --version'
                subprocess.check_call(exec_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                sys.stderr.write('Warning: GEOPM_RM undefined and unrecognized host: "{hh}", using SLURM\n'.format(hh=hostname))
                result = "SLURM"
            except subprocess.CalledProcessError:
                try:
                    exec_str = 'aprun --version'
                    subprocess.check_call(exec_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                    sys.stderr.write("Warning: GEOPM_RM undefined and unrecognized host: \"{hh}\", using ALPS\n".format(hh=hostname))
                    result = "ALPS"
                except subprocess.CalledProcessError:
                    raise LookupError('Unable to determine resource manager, set GEOPM_RM environment variable to "SLURM" or "ALPS"')

    return result;

def factory(argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
            time_limit=None, job_name=None, node_list=None, host_file=None):
    rm = resource_manager()
    if rm == "SLURM":
        return SrunLauncher(argv[1:], num_rank, num_node, cpu_per_rank, timeout,
                            time_limit, job_name, node_list, host_file)
    elif rm == "ALPS":
        return AprunLauncher(argv[1:], num_rank, num_node, cpu_per_rank, timeout,
                             time_limit, job_name, node_list, host_file)

class PassThroughError(Exception):
    """
    Exception raised when geopm is not to be used.
    """

class SubsetOptionParser(optparse.OptionParser):
    """
    Parse a subset of comand line arguments and prepend unrecognized
    arguments to positional arguments.
    """
    def _process_args(self, largs, rargs, values):
        while rargs:
            try:
                optparse.OptionParser._process_args(self, largs, rargs, values)
            except (optparse.BadOptionError, optparse.AmbiguousOptionError) as e:
                largs.append(e.opt_str)

class Config(object):
    def __init__(self, argv):
        self.ctl = None
        if not any(aa.startswith('--geopm-ctl') for aa in argv):
            raise PassThroughError('The --geopm-ctl flag is not specified.')
        # Parse the subset of arguements used by geopm
        parser = SubsetOptionParser()
        parser.add_option('--geopm-ctl', dest='ctl', nargs=1, type='string')
        parser.add_option('--geopm-policy', dest='policy', nargs=1, type='string')
        parser.add_option('--geopm-report', dest='report', nargs=1, type='string')
        parser.add_option('--geopm-trace', dest='trace', nargs=1, type='string')
        parser.add_option('--geopm-shmkey', dest='shmkey', nargs=1, type='string')
        parser.add_option('--geopm-timeout', dest='timeout', nargs=1, type='string')
        parser.add_option('--geopm-plugin', dest='plugin', nargs=1, type='string')
        parser.add_option('--geopm-debug-attach', dest='debug_attach', nargs=1, type='string')
        parser.add_option('--geopm-barrier', dest='barrier', action='store_true', default=False)
        parser.add_option('--geopm-preload', dest='preload', action='store_true', default=False)
        opts, self.args = parser.parse_args(argv)
        # Error check inputs
        if opts.ctl not in ('process', 'pthread', 'application'):
            raise SyntaxError('--geopm-ctl must be one of: "process", "pthread", or "application"')
        # copy opts object into self
        self.ctl = opts.ctl
        self.policy = opts.policy
        self.report = opts.report
        self.trace = opts.trace
        self.shmkey = opts.shmkey
        self.timeout = opts.timeout
        self.plugin = opts.plugin
        self.debug_attach = opts.debug_attach
        self.barrier = opts.barrier
        self.preload = opts.preload

    def __repr__(self):
        return ' '.join(['{kk}={vv}'.format(kk=kk, vv=vv)
                         for (kk, vv) in self.environ().iteritems()])

    def __str__(self):
        return self.__repr__()

    def environ(self):
        result = {'LD_DYNAMIC_WEAK':'true'}
        if self.ctl in ('process', 'pthread'):
            result['GEOPM_PMPI_CTL'] = self.ctl
        if self.policy:
            result['GEOPM_POLICY'] = self.policy
        if self.report:
            result['GEOPM_REPORT'] = self.report
        if self.trace:
            result['GEOPM_TRACE'] = self.trace
        if self.shmkey:
            result['GEOPM_SHMKEY'] = self.shmkey
        if self.timeout:
            result['GEOPM_PROFILE_TIMEOUT'] = self.timeout
        if self.plugin:
            result['GEOPM_PLUGIN_PATH'] = self.timeout
        if self.debug_attach:
            result['GEOPM_DEBUG_ATTACH'] = self.debug_attach
        if self.barrier:
            result['GEOPM_REGION_BARRIER'] = 'true'
        if self.preload:
            result['LD_PRELOAD'] = ':'.join((ll for ll in
                                   ('libgeopm.so', os.getenv('LD_PRELOAD'))
                                   if ll is not None))
        return result

    def unparsed(self):
        return self.args

class Launcher(object):
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, host_file=None):
        self.default_handler = signal.getsignal(signal.SIGINT)
        try:
            self.config = Config(argv)
            self.argv = self.config.unparsed()
            self.num_rank = num_rank
            self.num_node = num_node
            if self.num_rank and self.num_node:
                self.rank_per_node = int(math.ceil(float(self.num_rank) / float(self.num_node)))
            else:
                self.rank_per_node = None
            self.cpu_per_rank = cpu_per_rank
            self.timeout = timeout
            self.time_limit = time_limit
            self.job_name = job_name
            self.node_list = node_list
            self.host_file = host_file
            self.parse_alloc()
        except PassThroughError:
            self.config = None
            self.argv = argv

    def run(self, stdout=sys.stdout, stderr=sys.stderr):
        argv_mod = self.mpiexec()
        argv_mod.extend(self.mpiexec_argv())
        argv_mod.extend(self.argv)
        echo = []
        if self.config is not None:
            echo.append(self.config.__str__())
        echo.extend(argv_mod)
        echo = '\n' + ' '.join(echo) + '\n\n'
        stdout.write(echo)
        stdout.flush()
        signal.signal(signal.SIGINT, self.int_handler)
        pid = subprocess.Popen(argv_mod, env=self.environ(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout_str, stderr_str = pid.communicate()
        signal.signal(signal.SIGINT, self.default_handler)
        stdout.write(stdout_str)
        stderr.write(stderr_str)
        if pid.returncode:
            raise subprocess.CalledProcessError(pid.returncode, argv_mod)

    def environ(self):
        result = dict(os.environ)
        if self.config is not None:
            result.update(self.config.environ())
            if ('OMP_NUM_THREADS' not in result.keys() and
               self.cpu_per_rank is not None):
               result['OMP_NUM_THREADS'] = str(self.cpu_per_rank)

        return result

    def mod_alloc(self):
        self.num_app_rank = self.num_rank
        self.num_app_mask = int(math.ceil(float(self.num_app_rank) / float(self.num_node)))

        if self.cpu_per_rank is None:
            self.cpu_per_rank = int(os.environ.get('OMP_NUM_THREADS', '1'))

        if self.config.ctl == 'process':
            self.num_rank += self.num_node
            self.rank_per_node += 1

    def int_handler(self, signum, frame):
        return self.default_handler(signum, frame)

    def parse_alloc(self):
        raise NotImplementedError('Launcher.parse_alloc() undefined in the base class')

    def mpiexec(self):
        raise NotImplementedError('Launcher.mpiexec() undefined in the base class')

    def mpiexec_argv(self):
        result = []
        if self.config is not None:
            result.extend(self.num_node_option())
            result.extend(self.num_rank_option())
            result.extend(self.affinity_option())
            result.extend(self.timeout_option())
            result.extend(self.time_limit_option())
            result.extend(self.job_name_option())
            result.extend(self.node_list_option())
            result.extend(self.host_file_option())
        return result

    def num_node_option(self):
        raise NotImplementedError('Launcher.num_node_option() undefined in the base class')

    def num_rank_option(self):
        raise NotImplementedError('Launcher.num_rank_option() undefined in the base class')

    def affinity_option(self):
        return []

    def timeout_option(self):
        return []

    def time_limit_option(self):
        return []

    def job_name_option(self):
        return []

    def node_list_option(self):
        return []

    def host_file_option(self):
        return []

    def get_idle_nodes(self):
        raise NotImplementedError('Launcher.get_idle_nodes() undefined in the base class')

    def get_alloc_nodes(self):
        raise NotImplementedError('Launcher.get_alloc_nodes() undefined in the base class')


class SrunLauncher(Launcher):
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, host_file=None):
        super(SrunLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                           time_limit, job_name, node_list, host_file)

    def int_handler(self, signum, frame):
        """
        This is necessary to prevent the script from dying on the first
        CTRL-C press.  SLURM requires 2 SIGINT signals to abort the
        job.
        """
        sys.stderr.write("srun: interrupt (one more within 1 sec to abort)\n")

    def parse_alloc(self):
        # Parse the subset of arguements used by geopm
        parser = SubsetOptionParser()
        parser.add_option('-n', '--ntasks', dest='num_rank', nargs=1, type='int')
        parser.add_option('-N', '--nodes', dest='num_node', nargs=1, type='int')
        parser.add_option('-c', '--cpus-per-task', dest='cpu_per_rank', nargs=1, type='int')
        parser.add_option('-I', '--immediate', dest='timeout', nargs=1, type='int')
        parser.add_option('-t', '--time', dest='time_limit', nargs=1, type='int')
        parser.add_option('-J', '--job-name', dest='job_name', nargs=1, type='string')
        parser.add_option('-w', '--nodelist', dest='node_list', nargs=1, type='string')
        opts, self.argv = parser.parse_args(self.argv)

        if self.num_rank is None:
            self.num_rank = opts.num_rank
        if self.num_node is None:
            self.num_node = opts.num_node
        if self.rank_per_node is None and self.num_rank is not None and self.num_node is not None:
            self.rank_per_node = int(math.ceil(float(self.num_rank) / float(self.num_node)))
        if self.cpu_per_rank is None:
            self.cpu_per_rank = opts.cpu_per_rank
        if self.timeout is None:
            self.timeout = opts.timeout
        if self.time_limit is None:
            self.time_limit = opts.time_limit
        if self.job_name is None:
            self.job_name = opts.job_name
        if self.node_list is None:
            self.node_list = opts.node_list

        # Check required arguements
        if self.num_rank is None:
            raise SyntaxError('Number of tasks must be specified with -n.')
        if self.num_node is None:
            raise SyntaxError('Number of nodes must be specified with -N.')
        if any(aa.startswith('--cpu_bind') for aa in self.argv):
            raise SyntaxError('The option --cpu_bind must not be specified, this is controlled by geopm_srun.')

        self.mod_alloc()

    def mpiexec(self):
        return ['srun']

    def num_node_option(self):
        return ['-N', str(self.num_node)]

    def num_rank_option(self):
        return ['-n', str(self.num_rank)]

    def affinity_option(self):
        if self.config.ctl == 'application':
            raise NotImplementedError('Launch with geopmctl not supported')
        result = []
        pid = subprocess.Popen(['srun', '--help'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = pid.communicate()

        if out.find('--cpu_bind') != -1:
            result.append('--cpu_bind')
            mask_list = []
            if self.config.ctl == 'process':
                mask_list.append('0x1')
                binary_mask = self.cpu_per_rank * '1' + '0'
            elif self.config.ctl == 'pthread':
                binary_mask = (self.cpu_per_rank + 1) * '1'
            for ii in range(self.num_app_mask):
                hex_mask = '0x{:x}'.format(int(binary_mask, 2))
                mask_list.append(hex_mask)
                if ii == 0 and self.config.ctl == 'pthread':
                    binary_mask = self.cpu_per_rank * '1' + '0'
                binary_mask = binary_mask + self.cpu_per_rank * '0'
            result.append('v,mask_cpu:' + ','.join(mask_list))
        elif out.find('--mpibind') != -1:
            result.append('--mpibind=vv.on')

        return result

    def timeout_option(self):
        if self.timeout is None:
           result = []
        else:
           result = ['-I' + str(self.timeout)]
        return result

    def time_limit_option(self):
        if self.time_limit is None:
            result = []
        else:
            result = ['-t', str(self.time_limit)]
        return result

    def job_name_option(self):
        if self.job_name is None:
            result = []
        else:
            result = ['-J', self.job_name]
        return result

    def node_list_option(self):
        if (self.node_list is not None and
            self.host_file is not None and
            self.node_list != self.host_file):
            raise SyntaxError('Node list and host name cannot both be specified.')

        if self.node_list is None and self.host_file is None:
            result = []
        elif self.node_list is not None:
            result = ['-w', self.node_list]
        elif self.host_file is not None:
            result = ['-w', self.host_file]
        return result

    def get_idle_nodes(self):
        return subprocess.check_output('sinfo -t idle -hNo %N', shell=True).splitlines()

    def get_alloc_nodes(self):
        return subprocess.check_output('sinfo -t alloc -hNo %N', shell=True).splitlines()

class AprunLauncher(Launcher):
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, host_file=None):
        super(AprunLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                            time_limit, job_name, node_list, host_file)

    def environ(self):
        result = super(AprunLauncher, self).environ()
        result['KMP_AFFINITY'] = 'disabled'
        return result

    def parse_alloc(self):
        # Parse the subset of arguements used by geopm
        parser = SubsetOptionParser()
        parser.add_option('-n', '--pes', dest='num_rank', nargs=1, type='int')
        parser.add_option('-N', '--pes-per-node', dest='rank_per_node', nargs=1, type='int')
        parser.add_option('-d', '--cpus-per-pe', dest='cpu_per_rank', nargs=1, type='int')
        parser.add_option('-t', '--cpu-time-limit', dest='time_limit', nargs=1, type='int')
        parser.add_option('-L', '--node-list', dest='node_list', nargs=1, type='string')
        parser.add_option('-l', '--node-list-file', dest='host_file', nargs=1, type='string')
        opts, self.argv = parser.parse_args(self.argv)

        if self.num_rank is None:
            self.num_rank = opts.num_rank
        if self.rank_per_node is None:
            self.rank_per_node = opts.rank_per_node
        if self.num_node is None and self.num_rank is not None and self.rank_per_node is not None:
            self.num_node = int(math.ceil(float(self.num_rank) /
                                          float(self.rank_per_node)))
        if self.cpu_per_rank is None:
            self.cpu_per_rank = opts.cpu_per_rank
        if self.time_limit is None:
            self.time_limit = opts.time_limit
        if self.node_list is None:
            self.node_list = opts.node_list
        if self.host_file is None:
            self.host_file = opts.host_file

        # Check required arguements
        if self.num_rank is None:
            raise SyntaxError('Number of tasks must be specified with -n.')
        if self.rank_per_node is None:
            raise SyntaxError('Number of tasks per node must be specified with -N.')
        if any(aa.startswith('--cpu-binding') or
               aa.startswith('-cc') for aa in self.argv):
            raise SyntaxError('The options --cpu-binding or -cc must not be specified, this is controlled by geopm_launcher.')

        self.mod_alloc()

    def mpiexec(self):
        return ['aprun']

    def num_node_option(self):
        return ['-N', str(int(math.ceil(float(self.num_rank) /
                                        float(self.num_node))))]

    def num_rank_option(self):
        return ['-n', str(self.num_rank)]

    def affinity_option(self):
        if self.config.ctl == 'application':
            raise NotImplementedError('Launch with geopmctl not supported')
        result = ['--cpu-binding']
        mask_list = []
        off_start = 1
        if self.config.ctl == 'process':
            mask_list.append('0')
        mask_list.extend(['{0}-{1}'.format(off, off + self.cpu_per_rank - 1)
                          for off in range(off_start, self.cpu_per_rank * self.num_app_mask +
                                           off_start, self.cpu_per_rank)])
        result.append(':'.join(mask_list))
        return result

    def time_limit_option(self):
        if self.time_limit is None:
            result = []
        else:
            result = ['-t', str(self.time_limit)]
        return result

    def node_list_option(self):
        if self.node_list is None:
            result = []
        else:
            result = ['-L', self.node_list]
        return result

    def host_file_option(self):
        if self.host_file is None:
            result = []
        else:
            result = ['-l', self.host_file]
        return result

def main():
    launcher = factory(sys.argv)
    launcher.run()

if __name__ == '__main__':
    err = 0
    try:
        main()
        # Print geopm help if it appears that documentation was requested
        # Note: if application uses -h as a parameter or some other corner
        # cases there will be an extraneous help text printed at the end
        # of the run.
        if '--help' in sys.argv or '-h' in sys.argv:
            sys.stdout.write(__doc__)

    except Exception as e:
        sys.stderr.write("<geopm_launcher> {err}\n".format(err=e))
        err = -1
    sys.exit(err)
