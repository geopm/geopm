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
      --geopm-timeout=sec     application waits "sec" seconds for handshake
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
import StringIO
import itertools

def resource_manager():
    """
    Heuristic to determine the resource manager used on the system.
    Returns either "SLURM" or "ALPS", otherwise a LookupError is
    raised.
    """
    slurm_hosts = ['mr-fusion']
    alps_hosts = ['theta']

    result = os.environ.get('GEOPM_RM')

    if not result:
        hostname = socket.gethostname()

        if sys.argv[0].endswith('srun'):
            result = 'SLURM'
        elif sys.argv[0].endswith('aprun'):
            result = 'ALPS'
        elif any(hostname.startswith(word) for word in slurm_hosts):
            result = 'SLURM'
        elif any(hostname.startswith(word) for word in alps_hosts):
            result = 'ALPS'
        else:
            try:
                exec_str = 'srun --version'
                subprocess.check_call(exec_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                sys.stderr.write('Warning: GEOPM_RM undefined and unrecognized host: "{hh}", using SLURM\n'.format(hh=hostname))
                result = 'SLURM'
            except subprocess.CalledProcessError:
                try:
                    exec_str = 'aprun --version'
                    subprocess.check_call(exec_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                    sys.stderr.write("Warning: GEOPM_RM undefined and unrecognized host: \"{hh}\", using ALPS\n".format(hh=hostname))
                    result = 'ALPS'
                except subprocess.CalledProcessError:
                    raise LookupError('Unable to determine resource manager, set GEOPM_RM environment variable to "SLURM" or "ALPS"')

    return result;

def factory(argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
            time_limit=None, job_name=None, node_list=None, host_file=None):
    """
    Factory that returns a Launcher object.  Class selection is based
    on return value of the geopm_launcher.resource_manager() function.
    """
    rm = resource_manager()
    if rm == 'SLURM':
        return SrunLauncher(argv[1:], num_rank, num_node, cpu_per_rank, timeout,
                            time_limit, job_name, node_list, host_file)
    elif rm == 'ALPS':
        return AprunLauncher(argv[1:], num_rank, num_node, cpu_per_rank, timeout,
                             time_limit, job_name, node_list, host_file)

class PassThroughError(Exception):
    """
    Exception raised when geopm is not to be used.
    """

class SubsetOptionParser(optparse.OptionParser):
    """
    OptionParser derived object that will parse a subset of command
    line arguments and prepend unrecognized arguments to positional
    arguments.  Disable help and version message features of the
    OptionParser, these will be handled by the underlying job
    launcher.  GEOPM command line option help is appended to the job
    launcher's help message.
    """
    def _process_args(self, largs, rargs, values):
        while rargs:
            try:
                optparse.OptionParser._process_args(self, largs, rargs, values)
            except (optparse.BadOptionError, optparse.AmbiguousOptionError) as e:
                largs.append(e.opt_str)

    def _add_help_option(self):
        pass

    def _add_version_option(self):
        pass

def int_ceil_div(aa, bb):
    """
    Shortcut for the ceiling of the ratio of two integers.
    """
    return int(math.ceil(float(aa) / float(bb)))

def range_str(values):
    """
    Take an iterable object containing integers and return a string of
    comma separated values and ranges given by a dash.
    Example:

    >>> geopm_launcher.range_str({1, 2, 3, 5, 7, 9, 10})
    '1-3,5,7,9-10'
    """
    result = []
    # Turn iterable into a sorted list.
    values = list(values)
    values.sort()
    # Group values with equal delta compared the sequence from zero to
    # N, these are sequential.
    for aa, bb in itertools.groupby(enumerate(values), lambda(xx, yy): yy - xx):
        bb = list(bb)
        # The range is from the smallest to the largest in the group.
        begin = bb[0][1]
        end = bb[-1][1]
        # If the group is of size one, the value has no neighbors
        if begin == end:
           result.append(str(begin))
        # Otherwise create a range from the smallest to the largest
        else:
           result.append('{}-{}'.format(begin, end))
    # Return a comma separated list
    return ','.join(result)

class Config(object):
    """
    GEOPM configuration object.  Used to interpret command line
    arguments to set GEOPM related environment variables.
    """
    def __init__(self, argv):
        """
        Parse subset of command line arguments to create the GEOPM
        object. See geopm_launcher.__doc__ for information about the
        options.
        """
        self.ctl = None
        if not any(aa.startswith('--geopm-ctl') for aa in argv):
            raise PassThroughError('The --geopm-ctl flag is not specified.')
        # Parse the subset of arguments used by geopm
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

        opts, self.unparsed_argv = parser.parse_args(argv)
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
        self.cpu_per_rank = None

    def __repr__(self):
        """
        String describing environment variables controlled by the
        configuration object.
        """
        return ' '.join(['{kk}={vv}'.format(kk=kk, vv=vv)
                         for (kk, vv) in self.environ().iteritems()])

    def __str__(self):
        """
        String describing environment variables controlled by the
        configuration object.
        """
        return self.__repr__()

    def environ(self):
        """
        Dictionary describing the environment variables controlled by the
        configuration object.
        """
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
            result['GEOPM_PLUGIN_PATH'] = self.plugin
        if self.debug_attach:
            result['GEOPM_DEBUG_ATTACH'] = self.debug_attach
        if self.barrier:
            result['GEOPM_REGION_BARRIER'] = 'true'
        if self.preload:
            result['LD_PRELOAD'] = ':'.join((ll for ll in
                                   ('libgeopm.so', os.getenv('LD_PRELOAD'))
                                   if ll is not None))
        if self.omp_num_threads:
            result['OMP_NUM_THREADS'] = self.omp_num_threads
        return result

    def unparsed(self):
        """
        All command line arguments that are not used to configure GEOPM.
        """
        return self.unparsed_argv

    def set_omp_num_threads(self, omp_num_threads):
        """
        Control the OMP_NUM_THREADS environment variable.
        """
        self.omp_num_threads = str(omp_num_threads)

class Launcher(object):
    """
    Abstract base class for MPI job launch application abstraction.
    Defines common methods used by all Launcher objects.
    """
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, host_file=None):
        """
        Constructor takes the command line options passed to the job
        launch application along with optional override values for
        command line options.  These override values enable the
        creation of an abstract launcher while specifying command line
        options without knowledge of the specific command line flags
        used to control the values.
        """
        self.rank_per_node = None
        self.default_handler = signal.getsignal(signal.SIGINT)
        self.num_rank = num_rank
        self.num_node = num_node
        self.argv = argv
        try:
            self.config = Config(argv)
            self.is_geopm_enabled = True
            self.argv = self.config.unparsed()
        except PassThroughError:
            self.config = None
            self.is_geopm_enabled = False
        self.parse_mpiexec_argv()

        # Override values if they are passed in construction call
        if num_rank is not None:
            self.num_rank = num_rank
        if num_node is not None:
            self.num_node = num_node
        if cpu_per_rank is not None:
            self.cpu_per_rank = cpu_per_rank
        if timeout is not None:
            self.timeout = timeout
        if time_limit is not None:
            self.time_limit = time_limit
        if job_name is not None:
            self.job_name = job_name
        if node_list is not None:
            if type(node_list) is list:
                self.node_list = ' '.join(node_list)
            else:
                self.node_list = node_list
        if host_file is not None:
            self.host_file = host_file

        # Calculate derived values
        if self.rank_per_node is None and self.num_rank and self.num_node:
            self.rank_per_node = int_ceil_div(self.num_rank, self.num_node)

        self.num_app_rank = self.num_rank
        self.num_app_mask = self.rank_per_node

        if self.cpu_per_rank is None:
            self.cpu_per_rank = int(os.environ.get('OMP_NUM_THREADS', '1'))

        # Initialize GEOPM required values
        if self.is_geopm_enabled:
            # Check required arguments
            if self.num_rank is None:
                raise SyntaxError('Number of MPI ranks must be specified.')
            if self.num_node is None:
                raise SyntaxError('Number of nodes must be specified.')
            self.init_topo()
            if self.config.ctl == 'process':
                self.num_rank += self.num_node
                self.rank_per_node += 1

    def run(self, stdout=sys.stdout, stderr=sys.stderr):
        """
        Execute the command given to constructor with modified command
        line options and environment variables.
        """
        argv_mod = [self.mpiexec()]
        argv_mod.extend(self.mpiexec_argv())
        argv_mod.extend(self.argv)
        echo = []
        if self.is_geopm_enabled:
            self.config.set_omp_num_threads(self.cpu_per_rank)
            echo.append(self.config.__str__())
        echo.extend(argv_mod)
        echo = '\n' + ' '.join(echo) + '\n\n'
        stdout.write(echo)
        stdout.flush()
        signal.signal(signal.SIGINT, self.int_handler)
        argv_mod = ' '.join(argv_mod)
        if 'fileno' in dir(stdout) and 'fileno' in dir(stderr):
            pid = subprocess.Popen(argv_mod, env=self.environ(),
                                   stdout=stdout, stderr=stderr, shell=True)
            pid.communicate()
        else:
            pid = subprocess.Popen(argv_mod, env=self.environ(),
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
            stdout_str, stderr_str = pid.communicate()
            stdout.write(stdout_str)
            stderr.write(stderr_str)
        signal.signal(signal.SIGINT, self.default_handler)
        if pid.returncode:
            raise subprocess.CalledProcessError(pid.returncode, argv_mod)

    def environ(self):
        """
        Returns the modified environment dictionary updated with GEOPM
        specific values.
        """
        result = dict(os.environ)
        if self.is_geopm_enabled:
            result.update(self.config.environ())
        return result

    def int_handler(self, signum, frame):
        """
        This interface enables specialized signal handling.  If not
        overridden by derived class, then the default signal handler
        is used.
        """
        return self.default_handler(signum, frame)

    def init_topo(self):
        """
        Determine the topology of the compute nodes that the job will be
        launched on.  This is used to inform CPU affinity assignment.
        """
        argv = ['dummy', 'lscpu']
        launcher = factory(argv, 1, 1)
        ostream = StringIO.StringIO()
        launcher.run(stdout=ostream)
        out = ostream.getvalue()
        cpu_tpc_core_socket = [int(line.split(':')[1])
                               for line in out.splitlines()
                               if line.find('CPU(s):') == 0 or
                                  line.find('Thread(s) per core:') == 0 or
                                  line.find('Core(s) per socket:') == 0 or
                                  line.find('Socket(s):') == 0]
        self.num_linux_cpu = cpu_tpc_core_socket[0]
        self.thread_per_core = cpu_tpc_core_socket[1]
        self.core_per_socket = cpu_tpc_core_socket[2]
        self.num_socket = cpu_tpc_core_socket[3]

    def affinity_list(self):
        """
        Returns CPU affinity prescription as a list of integer sets.  The
        list is over MPI ranks on a node from lowest to highest rank.
        The process for each MPI rank is restricted to the Linux CPUs
        enumerated in the set.  The output from this function is used
        by the derived class's affinity_option() method to set CPU
        affinities.
        """
        if self.config.ctl == 'application':
            raise NotImplementedError('Launch with geopmctl not supported')
        result = []
        app_cpu_per_node = self.num_app_mask * self.cpu_per_rank
        core_per_node = self.core_per_socket * self.num_socket
        app_thread_per_core = int_ceil_div(app_cpu_per_node, core_per_node)
        rank_per_socket = self.num_app_mask // self.num_socket

        if app_cpu_per_node > self.num_linux_cpu:
            raise RuntimeError('Requested more application threads per node than the number of Linux logical CPUs')
        if app_cpu_per_node % core_per_node == 0:
            sys.stderr.write("Warning: User requested all cores for application, GEOPM controller will share a core with the application.\n")
            off = 0
            if self.thread_per_core != app_thread_per_core:
                # run controller on the lowest hyperthread that is not
                # occupied by the application
                geopm_ctl_cpu = core_per_node * app_thread_per_core
            else:
                # Oversubscribe Linux CPU 0, no better solution
                geopm_ctl_cpu = 0
        elif app_cpu_per_node // app_thread_per_core == core_per_node - 1:
            # Application requested all but one core, run the
            # controller on Linux CPU 0
            off = 1
            geopm_ctl_cpu = 0
        else:
            # Application not using at least two cores, leave one for
            # OS and use the other for the controller
            off = 2
            geopm_ctl_cpu = 1

        if self.config.ctl == 'process':
            result.append({geopm_ctl_cpu})

        socket = 0
        for rank in range(self.num_app_mask):
            rank_mask = set()
            if rank == 0 and self.config.ctl == 'pthread':
                rank_mask.add(geopm_ctl_cpu)
            for thread in range(self.cpu_per_rank):
                rank_mask.add(off)
                for hthread in range(1, app_thread_per_core):
                    rank_mask.add(off + hthread * core_per_node)
                off += 1
            result.append(rank_mask)
            if rank != 0 and rank % rank_per_socket == 0:
                socket += 1
                off = cpu_per_socket * socket
        return result

    def parse_mpiexec_argv(self):
        """
        Parse command line options accepted by the underlying job launch
        application.
        """
        raise NotImplementedError('Launcher.parse_mpiexec_argv() undefined in the base class')

    def mpiexec(self):
        """
        Returns the name/path to the job launch application.
        """
        raise NotImplementedError('Launcher.mpiexec() undefined in the base class')

    def mpiexec_argv(self):
        """
        Returns a list of command line options for underlying job launch
        application that reflect the state of the Launcher object.
        """
        result = []
        result.extend(self.num_node_option())
        result.extend(self.num_rank_option())
        result.extend(self.cpu_per_rank_option())
        result.extend(self.affinity_option())
        result.extend(self.timeout_option())
        result.extend(self.time_limit_option())
        result.extend(self.job_name_option())
        result.extend(self.node_list_option())
        result.extend(self.host_file_option())
        return result

    def num_node_option(self):
        """
        Returns a list containing the command line options specifying the
        number of compute nodes.
        """
        raise NotImplementedError('Launcher.num_node_option() undefined in the base class')

    def num_rank_option(self):
        """
        Returns a list containing the command line options specifying the
        number of MPI processes or "ranks".
        """
        raise NotImplementedError('Launcher.num_rank_option() undefined in the base class')

    def cpu_per_rank_option(self):
        """
        Returns a list containing the command line options specifying the
        number of Linux CPUs reserved for each MPI process.
        """
        return []

    def affinity_option(self):
        """
        Returns a list containing the command line options specifying the
        the CPU affinity for each MPI process on a compute node.
        """
        return []

    def timeout_option(self):
        """
        Returns a list containing the command line options specifying the
        length of time to wait for a job to start before aborting.
        """
        return []

    def time_limit_option(self):
        """
        Returns a list containing the command line options specifying the
        maximum time that a job is allowed to run.
        """
        return []

    def job_name_option(self):
        """
        Returns a list containing the command line options specifying the
        name associated with the job for scheduler tracking purposes.
        """
        return []

    def node_list_option(self):
        """
        Returns a list containing the command line options specifying the
        names of the compute nodes for the job to run on.
        """
        return []

    def host_file_option(self):
        """
        Returns a list containing the command line options specifying the
        file containing the names of the compute nodes for the job to
        run on.
        """
        return []

    def get_idle_nodes(self):
        """
        Returns a list of the names of compute nodes that are currently
        available to run jobs.
        """
        raise NotImplementedError('Launcher.get_idle_nodes() undefined in the base class')

    def get_alloc_nodes(self):
        """
        Returns a list of the names of compute nodes that have been
        reserved by a scheduler for current job context.
        """
        raise NotImplementedError('Launcher.get_alloc_nodes() undefined in the base class')

class SrunLauncher(Launcher):
    """
    Launcher derived object for use with the SLURM job launch
    application srun.
    """
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, host_file=None):
        """
        Pass through to Launcher constructor.
        """
        super(SrunLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                           time_limit, job_name, node_list, host_file)

    def int_handler(self, signum, frame):
        """
        This is necessary to prevent the script from dying on the first
        CTRL-C press.  SLURM requires 2 SIGINT signals to abort the
        job.
        """
        sys.stderr.write("srun: interrupt (one more within 1 sec to abort)\n")

    def parse_mpiexec_argv(self):
        """
        Parse the subset of srun command line arguments used or
        manipulated by GEOPM.
        """
        parser = SubsetOptionParser()
        parser.add_option('-n', '--ntasks', dest='num_rank', nargs=1, type='int')
        parser.add_option('-N', '--nodes', dest='num_node', nargs=1, type='int')
        parser.add_option('-c', '--cpus-per-task', dest='cpu_per_rank', nargs=1, type='int')
        parser.add_option('-I', '--immediate', dest='timeout', nargs=1, type='int')
        parser.add_option('-t', '--time', dest='time_limit', nargs=1, type='int')
        parser.add_option('-J', '--job-name', dest='job_name', nargs=1, type='string')
        parser.add_option('-w', '--nodelist', dest='node_list', nargs=1, type='string')
        parser.add_option('--ntasks-per-node', dest='rank_per_node', nargs=1, type='int')

        opts, self.argv = parser.parse_args(self.argv)

        self.num_rank = opts.num_rank
        self.num_node = opts.num_node
        if self.num_rank is not None and self.num_node is not None:
            self.rank_per_node = int_ceil_div(self.num_rank, self.num_node)
        else:
            self.rank_per_node = opts.rank_per_node
            if self.num_node is None and self.num_rank is not None and self.rank_per_node is not None:
                self.num_node = int_ceil_div(self.num_rank, self.rank_per_node)
        self.cpu_per_rank = opts.cpu_per_rank
        self.timeout = opts.timeout
        self.time_limit = opts.time_limit
        self.job_name = opts.job_name
        self.node_list = opts.node_list # Note this may also be the host file
        self.host_file = None

        if (self.is_geopm_enabled and
            any(aa.startswith('--cpu_bind') for aa in self.argv)):
            raise SyntaxError('The option --cpu_bind must not be specified, this is controlled by geopm_srun.')

    def mpiexec(self):
        """
        Returns 'srun', the name of the SLURM MPI job launch application.
        """
        return 'srun'

    def num_node_option(self):
        """
        Returns a list containing the -N option for srun.
        """
        return ['-N', str(self.num_node)]

    def num_rank_option(self):
        """
        Returns a list containing the -n option for srun.
        """
        return ['-n', str(self.num_rank)]

    def cpu_per_rank_option(self):
        """
        Returns a list containing the -c option for srun.
        """
        result = []
        if not self.is_geopm_enabled and self.cpu_per_rank is not None:
            result = ['-c', str(self.cpu_per_rank)]
        return result

    def affinity_option(self):
        """
        Returns a list containing the --cpu_bind or --mpibind option for
        srun depending on which SLURM plugin is loaded.  If neither
        plugin is loaded it returns an empty list.
        """
        result = []
        if self.is_geopm_enabled:
            aff_list = self.affinity_list()
            pid = subprocess.Popen(['srun', '--help'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            help_msg, err = pid.communicate()

            if help_msg.find('--cpu_bind') != -1:
                num_mask = len(aff_list)
                mask_zero = ['0' for ii in range(self.num_linux_cpu)]
                mask_list = []
                for cpu_set in aff_list:
                    mask = list(mask_zero)
                    for cpu in cpu_set:
                        mask[self.num_linux_cpu - 1 - cpu] = '1'
                    mask = '0x{:x}'.format(int(''.join(mask), 2))
                    mask_list.append(mask)
                result.append('--cpu_bind')
                result.append('v,mask_cpu:' + ','.join(mask_list))
            elif help_msg.find('--mpibind') != -1:
                result.append('--mpibind=vv.on')
        return result

    def timeout_option(self):
        """
        Returns a list containing the -I option for srun.
        """
        if self.timeout is None:
           result = []
        else:
           result = ['-I' + str(self.timeout)]
        return result

    def time_limit_option(self):
        """
        Returns a list containing the -t option for srun.
        """
        if self.time_limit is None:
            result = []
        else:
            result = ['-t', str(self.time_limit)]
        return result

    def job_name_option(self):
        """
        Returns a list containing the -J option for srun.
        """
        if self.job_name is None:
            result = []
        else:
            result = ['-J', self.job_name]
        return result

    def node_list_option(self):
        """
        Returns a list containing the -w option for srun.
        """
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

        raise NotImplementedError('Launcher.get_idle_nodes() undefined in the base class')

    def get_alloc_nodes(self):
        raise NotImplementedError('Launcher.get_alloc_nodes() undefined in the base class')

    def get_idle_nodes(self):
        """
        Returns a list of the names of compute nodes that are currently
        available to run jobs using the sinfo command.
        """
        return subprocess.check_output('sinfo -t idle -hNo %N', shell=True).splitlines()

    def get_alloc_nodes(self):
        """
        Returns a list of the names of compute nodes that have been
        reserved by a scheduler for current job context using the
        sinfo command.

        """
        return subprocess.check_output('sinfo -t alloc -hNo %N', shell=True).splitlines()

class AprunLauncher(Launcher):
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, host_file=None):
        """
        Pass through to Launcher constructor.
        """
        super(AprunLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                            time_limit, job_name, node_list, host_file)

    def environ(self):
        """
        Pass through to Launcher.environ().  Additionally the KMP_AFFINITY
        environment variable is set to 'disabled' to avoid bad
        interactions between aprun and the Intel OpenMP runtime for
        thread CPU affinity assignment.
        """
        result = super(AprunLauncher, self).environ()
        result['KMP_AFFINITY'] = 'disabled'
        return result

    def parse_mpiexec_argv(self):
        """
        Parse the subset of aprun command line arguments used or
        manipulated by GEOPM.
        """
        parser = SubsetOptionParser()
        parser.add_option('-n', '--pes', dest='num_rank', nargs=1, type='int')
        parser.add_option('-N', '--pes-per-node', dest='rank_per_node', nargs=1, type='int')
        parser.add_option('-d', '--cpus-per-pe', dest='cpu_per_rank', nargs=1, type='int')
        parser.add_option('-t', '--cpu-time-limit', dest='time_limit', nargs=1, type='int')
        parser.add_option('-L', '--node-list', dest='node_list', nargs=1, type='string')
        parser.add_option('-l', '--node-list-file', dest='host_file', nargs=1, type='string')

        opts, self.argv = parser.parse_args(self.argv)

        self.num_rank = opts.num_rank
        self.rank_per_node = opts.rank_per_node
        if self.num_rank is not None and self.rank_per_node is not None:
            self.num_node = int_ceil_div(self.num_rank, self.rank_per_node)
        self.cpu_per_rank = opts.cpu_per_rank
        self.timeout = None
        self.time_limit = opts.time_limit
        self.node_list = opts.node_list
        self.host_file = opts.host_file

        if (self.is_geopm_enabled and
            any(aa.startswith('--cpu-binding') or
            aa.startswith('-cc') for aa in self.argv)):
            raise SyntaxError('The options --cpu-binding or -cc must not be specified, this is controlled by geopm_launcher.')

    def mpiexec(self):
        """
        Returns 'aprun', the name of the ALPS MPI job launch application.
        """
        return 'aprun'

    def num_node_option(self):
        """
        Returns a list containing the -N option for aprun.  Must be
        combined with the -n option to determine the number of nodes.
        """
        if self.num_rank is None or self.num_node is None:
            result = []
        else:
            result = ['-N', str(int_ceil_div(self.num_rank, self.num_node))]
        return result

    def num_rank_option(self):
        """
        Returns a list containing the -n option for aprun.
        """
        if self.num_rank is None:
            result = []
        else:
            result = ['-n', str(self.num_rank)]
        return result

    def cpu_per_rank_option(self):
        """
        Returns a list containing the -d option for aprun.
        """
        result = []
        if not self.is_geopm_enabled and self.cpu_per_rank is not None:
            result = ['-d', str(self.cpu_per_rank)]
        return result

    def affinity_option(self):
        """
        Returns the --cpu-binding option for aprun.
        """
        result = []
        if self.is_geopm_enabled:
            result.append('--cpu-binding')
            aff_list = self.affinity_list()
            mask_list = [range_str(cpu_set) for cpu_set in aff_list]
            result.append(':'.join(mask_list))
        return result

    def time_limit_option(self):
        """
        Returns a list containing the -t option for aprun.
        """
        if self.time_limit is None:
            result = []
        else:
            result = ['-t', str(self.time_limit)]
        return result

    def node_list_option(self):
        """
        Returns a list containing the -L option for aprun.
        """
        if self.node_list is None:
            result = []
        else:
            result = ['-L', self.node_list]
        return result

    def host_file_option(self):
        """
        Returns a list containing the -l option for aprun.
        """
        if self.host_file is None:
            result = []
        else:
            result = ['-l', self.host_file]
        return result


def main():
    """
    Main routine when geopm_launcher.py is called as an executable.
    This function creates a launcher from the factory and calls the
    run method.  If help was requested on the command line then help
    from the underlying application launcher is printed and the help
    for the GEOPM extensions are appended.  Returns -1 and prints an
    error message if an error occurs.  If the GEOPM_DEBUG environment
    variable is set and an error occurs a complete stack trace will be
    printed.
    """
    err = 0
    try:
        launcher = factory(sys.argv)
        launcher.run()
        # Print geopm help if it appears that documentation was requested
        # Note: if application uses -h as a parameter or some other corner
        # cases there will be an extraneous help text printed at the end
        # of the run.
        if '--help' in sys.argv or '-h' in sys.argv:
            sys.stdout.write(__doc__)
    except Exception as e:
        # If GEOPM_DEBUG environment variable is defined print stack trace.
        if os.getenv('GEOPM_DEBUG'):
            raise
        sys.stderr.write("<geopm_launcher> {err}\n".format(err=e))
        err = -1
    return err

if __name__ == '__main__':
    err = main()
    sys.exit(err)
