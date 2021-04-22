#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

"""This module provides a way to launch MPI applications using the
GEOPM runtime by wrapping the call to the system MPI application
launcher.  The module currently supports wrapping the SLURM 'srun'
command, the ALPS 'aprun' command, and 'mpiexec' command provided by
Open MPI and Intel MPI.  The primary use of this module
is through the geopmlauncher(1) command line executable which calls
the geopmpy.launcher.main() function.  See the geopmlauncher(1) man
page for details about the command line interface.
"""

from __future__ import absolute_import
from __future__ import division

import sys
import os
import argparse
import subprocess
import math
import signal
import itertools
import glob
import shlex
import textwrap
import io
import locale
import pipes
import socket
import tempfile
import traceback

from collections import OrderedDict
from geopmpy import __version__


class Factory(object):
    def __init__(self):
        self._launcher_dict = OrderedDict([('srun', SrunLauncher),
                                           ('SrunLauncher', SrunLauncher),
                                           ('aprun', AprunLauncher),
                                           ('AprunLauncher', AprunLauncher),
                                           ('impi', IMPIExecLauncher),
                                           ('IMPIExecLauncher', IMPIExecLauncher),
                                           ('SrunTOSSLauncher', SrunTOSSLauncher),
                                           ('ompi', OMPIExecLauncher),
                                           ('OMPIExecLauncher', OMPIExecLauncher)])

    def create(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
               time_limit=None, job_name=None, node_list=None, exclude_list=None, host_file=None,
               reservation=None, quiet=None):
        try:
            launcher_name = argv[1]
            return self._launcher_dict[launcher_name](argv[2:], num_rank, num_node, cpu_per_rank, timeout,
                                                      time_limit, job_name, node_list, exclude_list,
                                                      host_file, reservation=reservation, quiet=quiet)
        except KeyError:
            raise LookupError('<geopm> geopmpy.launcher: Unsupported launcher "{}" requested'.format(launcher_name))

    def get_launcher_names(self):
        return list(self._launcher_dict)


class PassThroughError(Exception):
    """
    Exception raised when geopm is not to be used.
    """


def int_ceil_div(aa, bb):
    """
    Shortcut for the ceiling of the ratio of two integers.
    """
    return int(math.ceil(aa / bb))


def range_str(values):
    """
    Take an iterable object containing integers and return a string of
    comma separated values and ranges given by a dash.
    Example:

    >>> geopmpy.launcher.range_str({1, 2, 3, 5, 7, 9, 10})
    '1-3,5,7,9-10'
    """
    result = []
    # Turn iterable into a sorted list.
    values = list(values)
    values.sort()
    # Group values with equal delta compared the sequence from zero to
    # N, these are sequential.
    for aa, bb in itertools.groupby(enumerate(values), lambda xx_yy: xx_yy[1] - xx_yy[0]):
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
        object. See geopmpy.launcher.__doc__ for information about the
        options.
        """
        self.ctl = None
        if any(aa.startswith('--geopm-ctl-disable') for aa in argv):
            argv.remove('--geopm-ctl-disable')
            if any(aa.startswith('--geopm-') for aa in argv):
                raise RuntimeError('<geopm> geopmpy.launcher: Some GEOPM options have been provided but --geopm-ctl-disable was also given.')
            raise PassThroughError('<geopm> geopmpy.launcher: --geopm-ctl-disable specified; disabling the controller...')
        # Parse the subset of arguments used by geopm
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('--geopm-report', dest='report', type=str, default='geopm.report')
        parser.add_argument('--geopm-report-signals', dest='report_signals', type=str)
        parser.add_argument('--geopm-trace', dest='trace', type=str)
        parser.add_argument('--geopm-trace-signals', dest='trace_signals', type=str)
        parser.add_argument('--geopm-trace-profile', dest='trace_profile', type=str)
        parser.add_argument('--geopm-trace-endpoint-policy', dest='trace_endpoint_policy', type=str)
        parser.add_argument('--geopm-profile', dest='profile', type=str)
        parser.add_argument('--geopm-ctl', dest='ctl', type=str, default='process')
        parser.add_argument('--geopm-agent', dest='agent', type=str)
        parser.add_argument('--geopm-policy', dest='policy', type=str)
        parser.add_argument('--geopm-shmkey', dest='shmkey', type=str)
        parser.add_argument('--geopm-timeout', dest='timeout', type=str)
        parser.add_argument('--geopm-endpoint', dest='endpoint', type=str)
        parser.add_argument('--geopm-plugin-path', dest='plugin', type=str)
        parser.add_argument('--geopm-debug-attach', dest='debug_attach', type=str)
        parser.add_argument('--geopm-preload', dest='preload', action='store_true', default=False)
        parser.add_argument('--geopm-hyperthreads-disable', dest='allow_ht_pinning', action='store_false', default=True)
        parser.add_argument('--geopm-ompt-disable', dest='ompt_disable', action='store_true', default=False)
        parser.add_argument('--geopm-record-filter', dest='record_filter', type=str)
        parser.add_argument('--geopm-affinity-disable', dest='do_affinity', action='store_false', default=True)
        opts, self.argv_unparsed = parser.parse_known_args(argv)
        # Error check inputs
        if opts.ctl not in ('process', 'pthread', 'application'):
            raise SyntaxError('<geopm> geopmpy.launcher: --geopm-ctl must be one of: "process", "pthread", or "application"')
        # copy opts object into self
        self.ctl = opts.ctl
        self.policy = opts.policy
        self.endpoint = opts.endpoint
        self.report = opts.report
        self.trace = opts.trace
        self.trace_profile = opts.trace_profile
        self.trace_endpoint_policy = opts.trace_endpoint_policy
        self.trace_signals = opts.trace_signals
        self.report_signals = opts.report_signals
        self.agent = opts.agent
        self.profile = opts.profile
        self.shmkey = opts.shmkey
        self.timeout = opts.timeout
        self.plugin = opts.plugin
        self.debug_attach = opts.debug_attach
        if opts.preload:
            sys.stderr.write("Warning: <geopmpy.launcher> The --geopm-preload option is deprecated, libgeopm is always preloaded.\n")
        self.preload = True
        self.omp_num_threads = None
        self.allow_ht_pinning = opts.allow_ht_pinning and 'GEOPM_DISABLE_HYPERTHREADS' not in os.environ
        self.ompt_disable = opts.ompt_disable
        self.record_filter = opts.record_filter
        self.do_affinity = opts.do_affinity

    def __repr__(self):
        """
        String describing environment variables controlled by the
        configuration object.
        """
        return ' '.join(['{kk}={vv}'.format(kk=kk, vv=vv)
                         for (kk, vv) in self.environ().items()])

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
        result = {'OMP_PROC_BIND': 'true'}
        if self.ctl == 'pthread':
            result['MPICH_MAX_THREAD_SAFETY'] = 'multiple'
        if self.ctl in ('process', 'pthread'):
            result['GEOPM_CTL'] = self.ctl
        if self.profile:
            result['GEOPM_PROFILE'] = self.profile
        else:
            result['GEOPM_PROFILE'] = ''
        if self.agent:
            result['GEOPM_AGENT'] = self.agent
        if self.policy:
            result['GEOPM_POLICY'] = self.policy
        if self.endpoint:
            result['GEOPM_ENDPOINT'] = self.endpoint
        if self.report:
            result['GEOPM_REPORT'] = self.report
        if self.trace:
            result['GEOPM_TRACE'] = self.trace
        if self.trace_profile:
            result['GEOPM_TRACE_PROFILE'] = self.trace_profile
        if self.trace_endpoint_policy:
            result['GEOPM_TRACE_ENDPOINT_POLICY'] = self.trace_endpoint_policy
        if self.trace_signals:
            result['GEOPM_TRACE_SIGNALS'] = self.trace_signals
        if self.report_signals:
            result['GEOPM_REPORT_SIGNALS'] = self.report_signals
        if self.shmkey:
            result['GEOPM_SHMKEY'] = self.shmkey
        if self.timeout:
            result['GEOPM_TIMEOUT'] = self.timeout
        if self.plugin:
            result['GEOPM_PLUGIN_PATH'] = self.plugin
        if self.debug_attach:
            result['GEOPM_DEBUG_ATTACH'] = self.debug_attach
        if self.omp_num_threads:
            result['OMP_NUM_THREADS'] = self.omp_num_threads
        if self.ompt_disable:
            result['GEOPM_OMPT_DISABLE'] = 'true'
        if self.record_filter:
            result['GEOPM_RECORD_FILTER'] = self.record_filter

        # Add geopm installed OpenMP library to LD_LIBRARY_PATH if it
        # is present.
        libdir = []
        path = os.path.realpath(__file__)
        index = path.rfind('/lib')
        if index != -1:
            libdir = glob.glob(path[:index] + '/lib*/geopm/openmp/lib')
        else:
            # If we are working from the source repo look for the local copy of OpenMP
            index = path.rfind('/scripts/geopmpy')
            if index != -1:
                libdir = glob.glob(path[:index] + '/openmp/lib')
        if len(libdir) == 1:
            # Avoid putting the CWD in the path if the path is not currently set
            result['LD_LIBRARY_PATH'] = ':'.join((ll for ll in
                                                  (libdir[0], os.getenv('LD_LIBRARY_PATH'))
                                                  if ll is not None))
        return result

    def unparsed(self):
        """
        All command line arguments except those used to configure GEOPM.
        """
        return self.argv_unparsed

    def set_omp_num_threads(self, omp_num_threads):
        """
        Control the OMP_NUM_THREADS environment variable.
        """
        self.omp_num_threads = str(omp_num_threads)

    def get_ctl(self):
        """
        Returns the geopm control method.
        """
        return self.ctl

    def get_policy(self):
        """
        Returns the geopm policy file/key.
        """
        return self.policy

    def get_shmkey(self):
        """
        Returns the geopm shared memory key base.
        """
        return self.shmkey

    def get_preload(self):
        """
        Returns True/False if the geopm preload option was specified or not.
        """
        return self.preload


class Launcher(object):
    """
    Abstract base class for MPI job launch application abstraction.
    Defines common methods used by all Launcher objects.
    """
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, exclude_list=None, host_file=None,
                 partition=None, reservation=None, quiet=None, do_affinity=None):
        """
        Constructor takes the command line options passed to the job
        launch application along with optional override values for
        command line options.  These override values enable the
        creation of an abstract launcher while specifying command line
        options without knowledge of the specific command line flags
        used to control the values.
        """
        self.environ_ext = dict()
        self.rank_per_node = None
        self.exclude_list = None
        self.reservation = None
        self.quiet = quiet
        self.default_handler = signal.getsignal(signal.SIGINT)
        self.num_rank = num_rank
        self.num_node = num_node
        self.argv = argv
        self.argv_unparsed = argv
        try:
            self.config = Config(argv)
            self.is_geopm_enabled = True
            self.is_override_enabled = True
            self.argv_unparsed = self.config.unparsed()
        except PassThroughError:
            self.config = None
            self.is_geopm_enabled = False
            self.is_override_enabled = False
        self.is_slurm_enabled = False
        self.parse_launcher_argv()

        self.cpu_per_rank = None
        is_cpu_per_rank_override = False

        # Override values if they are passed in construction call
        if num_rank is not None:
            self.is_override_enabled = True
            self.num_rank = num_rank
        if num_node is not None:
            self.is_override_enabled = True
            self.num_node = num_node
        if cpu_per_rank is not None:
            self.is_override_enabled = True
            is_cpu_per_rank_override = True
            self.cpu_per_rank = cpu_per_rank
        if timeout is not None:
            self.is_override_enabled = True
            self.timeout = timeout
        if time_limit is not None:
            self.is_override_enabled = True
            self.time_limit = time_limit
        if job_name is not None:
            self.is_override_enabled = True
            self.job_name = job_name
        if node_list is not None:
            self.is_override_enabled = True
            if type(node_list) is list:
                self.node_list = self.node_list_delim().join(node_list)
            else:
                self.node_list = node_list
        if exclude_list is not None:
            self.is_override_enabled = True
            if type(exclude_list) is list:
                self.exclude_list = self.node_list_delim().join(exclude_list)
            else:
                self.exclude_list = exclude_list
        if host_file is not None:
            self.is_override_enabled = True
            self.host_file = host_file
        if partition is not None:
            self.is_override_enabled = True
            self.partition = partition
        if reservation is not None:
            self.is_override_enabled = True
            self.reservation = reservation
        if do_affinity is not None:
            self.is_override_enabled = True
            self.do_affinity = do_affinity

        # Calculate derived values
        if self.rank_per_node is None and self.num_rank and self.num_node:
            self.rank_per_node = int_ceil_div(self.num_rank, self.num_node)

        if self.num_node is None and self.num_rank and self.rank_per_node:
            self.num_node = int_ceil_div(self.num_rank, self.rank_per_node)

        if os.getenv('SLURM_NNODES'):
            self.is_slurm_enabled = True
        if (self.is_geopm_enabled and
            self.is_slurm_enabled and
            self.config.get_ctl() == 'application' and
            os.getenv('SLURM_NNODES') != str(self.num_node)):
            raise RuntimeError('When using srun and specifying --geopm-ctl=application call must be made ' +
                               'inside of an salloc or sbatch environment and application must run on all allocated nodes.')

        self.num_app_rank = self.num_rank
        self.num_app_mask = self.rank_per_node

        if self.cpu_per_rank is None:
            self.cpu_per_rank = int(os.environ.get('OMP_NUM_THREADS', '1'))

        # Initialize GEOPM required values
        if self.is_geopm_enabled:
            # Check required arguments
            if self.num_rank is None:
                raise SyntaxError('<geopm> geopmpy.launcher: Number of MPI ranks must be specified.')
            if self.num_node is None:
                raise SyntaxError('<geopm> geopmpy.launcher: Number of nodes must be specified.')
            self.init_topo()
            if not is_cpu_per_rank_override and 'OMP_NUM_THREADS' not in os.environ:
                # exclude 2 cores for GEOPM and OS
                available_cores = self.num_linux_cpu // self.thread_per_core - 2
                core_per_rank = available_cores // self.rank_per_node
                if self.config.allow_ht_pinning:
                    self.cpu_per_rank = core_per_rank * self.thread_per_core
                else:
                    self.cpu_per_rank = core_per_rank
                if self.cpu_per_rank == 0:
                    self.cpu_per_rank = 1
                sys.stderr.write('Warning: <geopm> geopmpy.launcher: User did not provide OMP_NUM_THREADS, using {}\n'.format(self.cpu_per_rank))
            if self.config.get_ctl() == 'process':
                self.num_rank += self.num_node
                self.rank_per_node += 1

    def run(self, stdout=sys.stdout, stderr=sys.stderr):
        """
        Execute the command given to constructor with modified command
        line options and environment variables.

        Args:
            stdout (writable object): Destination for standard output
            stderr (writable object): Destination for standard error
        """
        # Output encodings may not be set, or may not be available through the
        # provided output object. Try to match the encoding if it is given,
        # otherwise use the current configured locale's encoding.
        try:
            stdout_encoding = stdout.encoding
        except AttributeError:
            stdout_encoding = None
        if stdout_encoding is None:
            stdout_encoding = locale.getpreferredencoding()

        try:
            stderr_encoding = stderr.encoding
        except AttributeError:
            stderr_encoding = None
        if stderr_encoding is None:
            stderr_encoding = locale.getpreferredencoding()

        argv_mod = [self.launcher_command()]
        if self.is_override_enabled:
            argv_mod.extend(self.launcher_argv(False))
            argv_mod.extend(self.argv_unparsed)
        else:
            argv_mod.extend(self.argv)
        echo = []
        if self.is_geopm_enabled:
            echo.append(str(self.config))
            for it in self.environ_ext.items():
                echo.append('{}={}'.format(it[0], it[1]))
        echo.extend(argv_mod)
        echo = u'\n' + u' '.join(echo) + u'\n\n'
        if not self.quiet:
            stdout.write(echo) # Echo the command that's about to be run
            stdout.flush()
        signal.signal(signal.SIGINT, self.int_handler)

        # Re-quote the arguments before concatenating them so we don't treat
        # any quoted words as separate args
        argv_mod = ' '.join(pipes.quote(arg) for arg in argv_mod)
        if self.is_geopm_enabled and self.config.get_ctl() == 'application':
            geopm_argv = [self.launcher_command()]
            geopm_argv.extend(self.launcher_argv(True))
            geopm_argv.append('geopmctl')
            geopm_argv = ' '.join(pipes.quote(arg) for arg in geopm_argv)
            is_geopmctl = True
        else:
            is_geopmctl = False

        # Popen stream redirection only works with things that can be written
        # through file descriptors. The launcher may be given a StringIO or some
        # other writable object without a file descriptor. Use PIPE and
        # communicate() so we can work in those cases.
        #
        # Why not just always use PIPE? It buffers all output until the process
        # terminates, which could be unwanted when live output is desired, and
        # can consume memory unnecessarily.
        try:
            popen_stdout = stdout.fileno()
            popen_stderr = stderr.fileno()
            if is_geopmctl:
                stdout.write("Controller launch config: {}\n".format(geopm_argv))
                stdout.flush()
        except (io.UnsupportedOperation, AttributeError):
            # StringIO.StringIO objects raise UnsupportedOperation, but
            # io.StringIO objects have no fileno() member.
            popen_stdout = subprocess.PIPE
            popen_stderr = subprocess.PIPE

        if is_geopmctl:
            # Need to set OMP_NUM_THREADS to 1 in the env before the run
            self.config.set_omp_num_threads(1)
            geopm_pid = subprocess.Popen(geopm_argv, env=self.environ(),
                                         stdout=popen_stdout, stderr=popen_stderr,
                                         shell=True)
        if self.is_geopm_enabled:
            self.config.set_omp_num_threads(self.cpu_per_rank)
        pid = subprocess.Popen(argv_mod, env=self.environ(),
                               stdout=popen_stdout, stderr=popen_stderr,
                               shell=True)
        stdout_bytes, stderr_bytes = pid.communicate()
        if subprocess.PIPE in (popen_stdout, popen_stderr):
            try:
                stdout.write(stdout_bytes.decode(encoding=stdout_encoding))
            except UnicodeEncodeError as u:
                stdout.write(stdout_bytes)
            try:
                stderr.write(stderr_bytes.decode(encoding=stderr_encoding))
            except UnicodeEncodeError as u:
                stderr.write(stderr_bytes)

        if is_geopmctl:
            stdout_bytes, stderr_bytes = geopm_pid.communicate()
            if subprocess.PIPE in (popen_stdout, popen_stderr):
                try:
                    stdout.write(stdout_bytes.decode(encoding=stdout_encoding))
                except UnicodeEncodeError as u:
                    stdout.write(stdout_bytes)
                try:
                    stderr.write(stderr_bytes.decode(encoding=stderr_encoding))
                except UnicodeEncodeError as u:
                    stderr.write(stderr_bytes)

        signal.signal(signal.SIGINT, self.default_handler)
        if pid.returncode:
            raise subprocess.CalledProcessError(pid.returncode, argv_mod)
        if is_geopmctl and geopm_pid.returncode:
            raise subprocess.CalledProcessError(geopm_pid.returncode, geopm_argv)

    def environ(self):
        """
        Returns the modified environment dictionary updated with GEOPM
        specific values.
        """
        result = dict(os.environ)
        if self.is_geopm_enabled:
            result.update(self.environ_ext)
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
        # Create the cache for the PlatformTopo on each compute node
        argv = shlex.split('dummy {} --geopm-ctl-disable -- geopmread --cache'.format(self.__class__.__name__))
        factory = Factory()
        # run geopmread --cache on every node (1 rank per node)
        launcher = factory.create(argv, num_rank=self.num_node, num_node=self.num_node,
                                  host_file=self.host_file,
                                  node_list=self.node_list,
                                  exclude_list=self.exclude_list,
                                  reservation=self.reservation)
        launcher.run()
        # Query the topology for Launcher calculations by running lscpu on one node.
        # Note that a warning may be emitted by underlying launcher when main application uses more
        # than one node and the node list is passed.  We should run lscpu on all the nodes in the
        # allocation and check that the node topology is uniform across all nodes used by the job
        # instead of just running on one node.
        argv = shlex.split('dummy {} --geopm-ctl-disable -- lscpu --hex'.format(self.__class__.__name__))
        # run lscpu on a single node (1 rank)
        launcher = factory.create(argv, 1, 1,
                                  host_file=self.host_file,
                                  node_list=self.node_list,
                                  exclude_list=self.exclude_list,
                                  reservation=self.reservation)
        ostream = io.StringIO()
        launcher.run(stdout=ostream)
        out = ostream.getvalue()
        cpu_tpc_core_socket = [int(line.split(':')[1])
                               for line in out.splitlines()
                               if line.find('CPU(s):') == 0 or
                                  line.find('Thread(s) per core:') == 0 or
                                  line.find('Core(s) per socket:') == 0 or
                                  line.find('Socket(s):') == 0]
        if len(cpu_tpc_core_socket) < 4:
            raise RuntimeError('<geopm> geopmpy.launcher: Unable to initialize platform topology with run command: '
                               + str(argv))
        self.num_linux_cpu = cpu_tpc_core_socket[0]
        self.thread_per_core = cpu_tpc_core_socket[1]
        self.core_per_socket = cpu_tpc_core_socket[2]
        self.num_socket = cpu_tpc_core_socket[3]

    def affinity_list(self, is_geopmctl):
        """
        Returns CPU affinity prescription as a list of integer sets.  The
        list is over MPI ranks on a node from lowest to highest rank.
        The process for each MPI rank is restricted to the Linux CPUs
        enumerated in the set.  The output from this function is used
        by the derived class's affinity_option() method to set CPU
        affinities.
        """
        app_rank_per_node = self.num_app_mask

        # The number of application logical CPUs per compute node.
        app_cpu_per_node = app_rank_per_node * self.cpu_per_rank
        # Total number of cores per node
        core_per_node = self.core_per_socket * self.num_socket

        # Number of application ranks per socket (floored)
        rank_per_socket = app_rank_per_node // self.num_socket
        rank_per_socket_remainder = app_rank_per_node % self.num_socket

        app_thread_per_core = 1
        while app_thread_per_core * core_per_node < app_cpu_per_node:
            app_thread_per_core += 1

        if app_thread_per_core > self.thread_per_core:
            err_fmt = '<geopm> geopmpy.launcher: Cannot oversubscribe hardware threads; requested threads per core: {}, hardware threads per core: {}.'
            raise RuntimeError(err_fmt.format(app_thread_per_core, self.thread_per_core))
        if app_rank_per_node > core_per_node:
            raise RuntimeError('<geopm> geopmpy.launcher: Cores cannot be shared between MPI ranks')
        if not self.config.allow_ht_pinning and app_thread_per_core > 1:
            raise RuntimeError('<geopm> geopmpy.launcher: Hyperthreads needed to satisfy ranks/threads configuration, but forbidden by'
                               ' --geopm-hyperthreads-disable.')
        if app_cpu_per_node > self.num_linux_cpu:
            raise RuntimeError('<geopm> geopmpy.launcher: Requested more application threads per node than the number of Linux logical CPUs')

        app_core_per_rank = self.cpu_per_rank // app_thread_per_core
        if self.cpu_per_rank % app_thread_per_core > 0:
            app_core_per_rank += 1

        if app_core_per_rank * app_rank_per_node > core_per_node:
            raise RuntimeError('<geopm> geopmpy.launcher: Cores cannot be shared between MPI ranks')

        result = []
        core_index = core_per_node - 1

        if rank_per_socket_remainder == 0:
            socket_boundary = self.core_per_socket * (self.num_socket - 1)
            for socket in range(self.num_socket - 1, -1, -1):
                for rank in range(rank_per_socket - 1, -1, -1):  # Start assigning ranks to cores from the highest rank/core backwards
                    base_cores = list(range(core_index, core_index - app_core_per_rank, -1))
                    cpu_range = set()
                    for ht in range(app_thread_per_core):
                        cpu_range.update({bc + ht * core_per_node for bc in base_cores})

                    if not is_geopmctl:
                        result.insert(0, cpu_range)
                    core_index -= app_core_per_rank
                if not (rank == 0 and socket == 0):
                    # Reset to highest core in the next socket when crossing the socket boundary
                    # unless we've pinned the last rank on the last socket
                    core_index = socket_boundary - 1

                socket_boundary -= self.core_per_socket
        else:
            for rank in range(app_rank_per_node - 1, -1, -1):  # Start assigning ranks to cores from the highest rank/core backwards
                base_cores = list(range(core_index, core_index - app_core_per_rank, -1))
                cpu_range = set()
                for ht in range(app_thread_per_core):
                    cpu_range.update({bc + ht * core_per_node for bc in base_cores})
                if not is_geopmctl:
                    result.insert(0, cpu_range)
                core_index -= app_core_per_rank

        if core_index <= 0:
            if self.config.allow_ht_pinning and core_per_node * app_thread_per_core < self.num_linux_cpu:
                # Run controller on the lowest hyperthread that is not
                # occupied by the application
                geopm_ctl_cpu = core_per_node * app_thread_per_core
            else:
                # Oversubscribe Linux CPU 0, no better solution
                geopm_ctl_cpu = 0
        else:
            geopm_ctl_cpu = 1

        if self.config.get_ctl() == 'process' or is_geopmctl:
            result.insert(0, {geopm_ctl_cpu})

        return result

    def parse_launcher_argv(self):
        """
        Parse command line options accepted by the underlying job launch
        application.
        """
        raise NotImplementedError('<geopm> geopmpy.launcher: Launcher.parse_launcher_argv() undefined in the base class')

    def launcher_command(self):
        """
        Returns the name/path to the job launch application.
        """
        raise NotImplementedError('<geopm> geopmpy.launcher: Launcher.launcher_command() undefined in the base class')

    def launcher_argv(self, is_geopmctl):
        """
        Returns a list of command line options for underlying job launch
        application that reflect the state of the Launcher object.
        """
        result = []
        result.extend(self.num_node_option())
        result.extend(self.exclude_list_option())
        result.extend(self.num_rank_option(is_geopmctl))
        if self.config and self.config.do_affinity:
            result.extend(self.affinity_option(is_geopmctl))
        result.extend(self.preload_option())
        result.extend(self.timeout_option())
        result.extend(self.time_limit_option())
        result.extend(self.job_name_option())
        result.extend(self.node_list_option())
        result.extend(self.host_file_option())
        result.extend(self.partition_option())
        result.extend(self.reservation_option())
        result.extend(self.performance_governor_option())
        return result

    def num_node_option(self):
        """
        Returns a list containing the command line options specifying the
        number of compute nodes.
        """
        raise NotImplementedError('<geopm> geopmpy.launcher: Launcher.num_node_option() undefined in the base class')

    def exclude_list_option(self):
        """
        Returns a list containing the command line options specifying the
        compute nodes to exclude from execution.
        """
        raise NotImplementedError('<geopm> geopmpy.launcher: Launcher.exclude_list_option() undefined in the base class')

    def num_rank_option(self, is_geopmctl):
        """
        Returns a list containing the -n option which is defined in the
        MPI standard for all job launch applications to specify the
        number of MPI processes or "ranks".
        """
        result = []
        if self.num_node is not None and self.num_rank is not None:
            result.append('-n')
            if is_geopmctl:
                result.append(str(self.num_node))
            else:
                result.append(str(self.num_rank))
        return result

    def affinity_option(self, is_geopmctl):
        """
        Returns a list containing the command line options specifying the
        the CPU affinity for each MPI process on a compute node.
        """
        return []

    def preload_option(self):
        if self.config and self.config.get_preload():
            self.environ_ext['LD_PRELOAD'] = ':'.join((ll for ll in
                                                       ('libgeopm.so', os.getenv('LD_PRELOAD'))
                                                       if ll is not None))
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

    def partition_option(self):
        """
        Returns a list containing the command line options specifiying the
        compute node partition for the job to run on.
        """
        return []

    def reservation_option(self):
        return []

    def performance_governor_option(self):
        """
        Returns a list containing the command line options specifying
        that the Linux power governor should be set to performance.
        """
        return []

    def get_idle_nodes(self):
        """
        Returns a list of the names of compute nodes that are currently
        available to run jobs.
        """
        raise NotImplementedError('<geopm> geopmpy.launcher: Launcher.get_idle_nodes() undefined in the base class')


    def get_alloc_nodes(self):
        """
        Returns a list of the names of compute nodes that have been
        reserved by a scheduler for current job context using the
        sinfo command.

        """
        if self.is_slurm_enabled:
            return subprocess.check_output('sinfo -t alloc -hNo %N', shell=True).decode().splitlines()
        else:
            raise NotImplementedError('Idle nodes feature requires use with SLURM')

    def node_list_delim(self):
        """
        Returns the delimiter that is to be used when constructing
        a node list string given a list of node names.
        """
        return ' '


class SrunLauncher(Launcher):
    """
    Launcher derived object for use with the SLURM job launch
    application srun.
    """
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, exclude_list=None, host_file=None,
                 reservation=None, quiet=None, do_affinity=None):
        """
        Pass through to Launcher constructor.
        """
        super(SrunLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                           time_limit, job_name, node_list, exclude_list, host_file,
                                           reservation=reservation, quiet=quiet, do_affinity=do_affinity)

        if (self.is_geopm_enabled and
            self.config.get_ctl() == 'application' and
            os.getenv('SLURM_NNODES') != str(self.num_node)):
            raise RuntimeError('<geopm> geopmpy.launcher: When using srun and specifying --geopm-ctl=application call must ' +
                               'be made inside of an salloc or sbatch environment and application must run on all allocated nodes.')

    def int_handler(self, signum, frame):
        """
        This is necessary to prevent the script from dying on the first
        CTRL-C press.  SLURM requires 2 SIGINT signals to abort the
        job.
        """
        sys.stderr.write("srun: interrupt (one more within 1 sec to abort)\n")

    def parse_launcher_argv(self):
        """
        Parse the subset of srun command line arguments used or
        manipulated by GEOPM.
        """
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('-n', '--ntasks', dest='num_rank', type=int)
        parser.add_argument('-N', '--nodes', dest='num_node', type=int)
        parser.add_argument('-x', '--exclude', dest='exclude_list', type=str)
        parser.add_argument('-c', '--cpus-per-task', dest='cpu_per_rank', type=int)
        parser.add_argument('-I', '--immediate', dest='timeout', type=int)
        parser.add_argument('-t', '--time', dest='time_limit', type=int)
        parser.add_argument('-J', '--job-name', dest='job_name', type=str)
        parser.add_argument('-w', '--nodelist', dest='node_list', type=str)
        parser.add_argument('--ntasks-per-node', dest='rank_per_node', type=int)
        parser.add_argument('-p', '--partition', dest='partition', type=str)
        parser.add_argument('--reservation', dest='reservation', type=str)

        opts, self.argv_unparsed = parser.parse_known_args(self.argv_unparsed)

        self.num_rank = opts.num_rank
        self.num_node = opts.num_node
        if self.num_rank is not None and self.num_node is not None:
            self.rank_per_node = int_ceil_div(self.num_rank, self.num_node)
        else:
            self.rank_per_node = opts.rank_per_node
            if self.num_node is None and self.num_rank is not None and self.rank_per_node is not None:
                self.num_node = int_ceil_div(self.num_rank, self.rank_per_node)
        self.exclude_list = opts.exclude_list
        self.cpu_per_rank = opts.cpu_per_rank
        self.timeout = opts.timeout
        self.time_limit = None
        if opts.time_limit is not None:
            # time limit in minutes, convert to seconds
            self.time_limit = opts.time_limit * 60
        self.job_name = opts.job_name
        self.node_list = opts.node_list  # Note this may also be the host file
        self.host_file = None
        self.partition = opts.partition
        self.reservation = opts.reservation

        if (self.is_geopm_enabled and
            self.config.do_affinity and
            any(aa.startswith(('--cpu_bind', '--cpu-bind')) for aa in self.argv)):
            raise SyntaxError('<geopm> geopmpy.launcher: The option --cpu_bind or --cpu-bind  must not be specified, this is controlled by geopm_srun.')

    def launcher_command(self):
        """
        Returns 'srun', the name of the SLURM MPI job launch application.
        """
        return 'srun'

    def num_node_option(self):
        """
        Returns a list containing the -N option for srun.
        """
        return ['-N', str(self.num_node)]

    def exclude_list_option(self):
        """
        Returns a list containing the -x option for srun.
        """
        result = []
        if self.exclude_list is not None:
            result = ['-x', self.exclude_list]
        return result

    def affinity_option(self, is_geopmctl):
        """
        Returns a list containing the --cpu_bind option for
        srun.  If the mpibind plugin is supported, it is explicitly
        disabled so it does not interfere with affinitization.  If
        the cpu_bind plugin is not detected, an exception is raised.
        """
        result = []
        if self.is_geopm_enabled:
            # Disable other affinity mechanisms
            self.environ_ext['KMP_AFFINITY'] = 'disabled'
            self.environ_ext['MV2_ENABLE_AFFINITY'] = '0'
            self.environ_ext['KMP_WARNINGS'] = 'FALSE'

            aff_list = self.affinity_list(is_geopmctl)
            pid = subprocess.Popen(['srun', '--help'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            help_msg, err = pid.communicate()
            if help_msg.find(b'--mpibind') != -1:
                result.append('--mpibind=off')
            if help_msg.find(b'--cpu_bind') != -1:
                bind_cmd = '--cpu_bind'
            elif help_msg.find(b'--cpu-bind') != -1:
                bind_cmd = '--cpu-bind'
            else:
                raise RuntimeError('<geopm> geopmpy.launcher: SLURM\'s cpubind plugin was not detected.  Unable to affinitize ranks.')

            mask_zero = ['0' for ii in range(self.num_linux_cpu)]
            mask_list = []
            for cpu_set in aff_list:
                mask = list(mask_zero)
                for cpu in cpu_set:
                    mask[self.num_linux_cpu - 1 - cpu] = '1'
                mask = '0x{:x}'.format(int(''.join(mask), 2))
                mask_list.append(mask)
            result.append(bind_cmd)
            result.append('v,mask_cpu:' + ','.join(mask_list))

        return result

    def timeout_option(self):
        """
        Returns a list containing the -I option for srun.
        """
        result = []
        if self.timeout is not None:
            result = ['-I' + str(self.timeout)]
        return result

    def time_limit_option(self):
        """
        Returns a list containing the -t option for srun.
        """
        result = []
        if self.time_limit is not None:
            # Time limit option exepcts minutes, covert from seconds
            result = ['-t', str(int_ceil_div(self.time_limit, 60))]
        return result

    def job_name_option(self):
        """
        Returns a list containing the -J option for srun.
        """

        result = []
        if self.job_name is not None:
            result = ['-J', self.job_name]
        return result

    def node_list_option(self):
        """
        Returns a list containing the -w option for srun.
        """
        if (self.node_list is not None and
            self.host_file is not None and
            self.node_list != self.host_file):
            raise SyntaxError('<geopm> geopmpy.launcher: Node list and host name cannot both be specified.')

        result = []
        if self.node_list is not None:
            result = ['-w', self.node_list]
        elif self.host_file is not None:
            result = ['-w', self.host_file]
        return result

    def partition_option(self):
        """
        Returns a list containing the command line options specifiying the
        compute node partition for the job to run on.
        """
        result = []
        if self.partition is not None:
            result = ['-p', self.partition]
        return result

    def reservation_option(self):
        result = []
        if self.reservation is not None:
            result = ['--reservation', self.reservation]
        return result

    def performance_governor_option(self):
        return ['--cpu-freq=Performance']

    def get_idle_nodes(self):
        """
        Returns a list of the names of compute nodes that are currently
        available to run jobs using the sinfo command.
        """
        return list(set(subprocess.check_output('sinfo -t idle -hNo %N', shell=True).decode().splitlines()))

    def get_alloc_nodes(self):
        """
        Returns a list of the names of compute nodes that have been
        reserved by a scheduler for current job context using the
        sinfo command.

        """
        return list(set(subprocess.check_output('scontrol show hostname', shell=True).decode().splitlines()))

    def preload_option(self):
        result = []
        if self.config and self.config.get_preload():
            value = ':'.join((ll for ll in
                              ('libgeopm.so', os.getenv('LD_PRELOAD'))
                              if ll is not None))
            result = ["--export=LD_PRELOAD={},ALL".format(value)]
        return result


class SrunTOSSLauncher(SrunLauncher):
    """
    Launcher derived object for use with systems using TOSS and the
    mpibind plugin from LLNL.
    """
    def affinity_option(self, is_geopmctl):
        """
        Returns the mpibind option used with SLURM on TOSS.
        """
        result = []
        if self.is_geopm_enabled:
            # Disable other affinity mechanisms
            self.environ_ext['KMP_AFFINITY'] = 'disabled'
            self.environ_ext['MV2_ENABLE_AFFINITY'] = '0'
            self.environ_ext['KMP_WARNINGS'] = 'FALSE'

            aff_list = self.affinity_list(is_geopmctl)
            mask_list = [range_str(cpu_set) for cpu_set in aff_list]
            result.append('--mpibind=v.' + ','.join(mask_list))
        return result

class OMPIExecLauncher(Launcher):
    """
    Launcher derived object for use with Open MPI project launch
    application mpiexec.
    """
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, exclude_list=None, host_file=None,
                 reservation=None, quiet=None, do_affinity=None):
        """
        Pass through to Launcher constructor.
        """
        super(OMPIExecLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                               time_limit, job_name, node_list, exclude_list,
                                               host_file, quiet=quiet, do_affinity=do_affinity)
        self._tmp_files = []

    def run(self, stdout=sys.stdout, stderr=sys.stderr):
        """
        Pass through to Launcher run.
        """
        try:
            super(OMPIExecLauncher, self).run(stdout, stderr)
        except:
            for ff in self._tmp_files:
                os.remove(ff)
            raise

    def launcher_command(self):
        """
        Returns 'mpiexec', the name of the Open MPI project job launch application.
        """
        return 'mpiexec'

    def parse_launcher_argv(self):
        """
        Parse the subset of mpiexec command line arguments used or
        manipulated by GEOPM.
        """
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('-n', dest='num_rank', type=int)
        parser.add_argument('-H', '--host', dest='node_list', type=str)
        parser.add_argument('--default-hostfile', dest='default_host_file', type=str)
        parser.add_argument('--hostfile', '--machinefile', dest='host_file', type=str)
        parser.add_argument('--npernode', dest='rank_per_node', type=int)

        #long form options whose short form options collide with short
        #form options we care about (short forms we care about {'-n'})
        long_form_options = ['-novm', '-npersocket', '-npernode',
                             '-nolocal', '-nooversubscribe', '-noprefix']
        for arg_idx in range(len(self.argv_unparsed)):
            for opt in long_form_options:
                if self.argv_unparsed[arg_idx].startswith(opt):
                    self.argv_unparsed[arg_idx] = '-' + self.argv_unparsed[arg_idx]
        opts, self.argv_unparsed = parser.parse_known_args(self.argv_unparsed)
        try:
            self.argv_unparsed.remove('--')
        except ValueError:
            pass

        self.host_file = None
        self.node_list = None
        if opts.num_rank:
            self.num_rank = opts.num_rank
        if opts.rank_per_node:
            self.rank_per_node = opts.rank_per_node
        if opts.node_list:
            self.node_list = opts.node_list
        if opts.host_file:
            self.host_file = opts.host_file
        if opts.host_file is None and opts.default_host_file:
            self.host_file = opts.default_host_file
        self.cpu_per_rank = None
        self.timeout = None
        self.time_limit = None
        self.job_name = None
        self.reservation = None

    def num_node_option(self):
        return []

    def launcher_argv(self, is_geopmctl):
        """
        Returns a list of command line options for underlying job launch
        application that reflect the state of the Launcher object.
        """
        result = super(OMPIExecLauncher, self).launcher_argv(is_geopmctl)

        if is_geopmctl:
            # add not to warn on fork messge
            result.append('--mca mpi_warn_on_fork 0')
        return result

    def parse_host_file(self, file_name):
        hostnames = []
        with open(file_name) as f:
            lines = f.readlines()
            for line in lines:
                if len(line) and not line.startswith('#'):
                    hostnames.append(line.split(' ')[0].strip('\n'))
        return hostnames

    def affinity_option(self, is_geopmctl):
        ret = []
        if self.is_geopm_enabled:
            # Disable other affinity mechanisms
            aff_list = self.affinity_list(is_geopmctl)
            # affinity_list assumes that the second half of the available logical
            # cpus are hyperthreads.  mpiexec from ompi encodes hyperthreads as
            # odd numbered cpu id's.  We create a map below to address this mismatch
            # in location of hyperthreads
            core_to_slot_map = list()
            physical_cores = list(range(0, self.num_linux_cpu, self.thread_per_core))
            ht_cores = list(range(1, self.num_linux_cpu, self.thread_per_core))
            core_to_slot_map.extend(physical_cores)
            core_to_slot_map.extend(ht_cores)

            hostnames = []
            if self.node_list:
                hostnames = [host.strip('\n') for host in self.node_list.split(',')]
            elif self.host_file:
                hostnames = self.parse_host_file(self.host_file)
            else:
                hostnames = [socket.gethostname()]

            rank = 0
            new_file, filename = tempfile.mkstemp()
            self._tmp_files.append(filename)

            # only warn of OpenMPI shortcoming once
            warned = False
            with os.fdopen(new_file, 'w') as f:
                for host in hostnames:
                    for cpu_set in aff_list:
                        cpu_str = ''
                        if len(cpu_set) == 1:
                            for cpu in cpu_set:
                                cpu_str = str(core_to_slot_map[cpu])
                        else:
                            converted = [str(core_to_slot_map[ii]) for ii in cpu_set]
                            cpu_str = ','.join(converted)

                            max_cores = 18
                            if len(converted) > max_cores and not warned:
                                sys.stderr.write('Warning: <geopm> geopmpy.launcher: Due to a bug in OpenMPI, you may ' +
                                                 'need to request {} cores per rank or less.'.format(max_cores) +
                                                 '  Currently requesting cpus_per_rank={}\n'.format(self.cpu_per_rank))
                                warned = True

                        line = 'rank ' + str(rank) + '=' + host + ' slot=' + cpu_str
                        if os.getenv('GEOPM_DEBUG'):
                            sys.stdout.write(line + '\n')
                        f.write(line)
                        f.write('\n')
                        rank += 1

            ret.extend(['--use-hwthread-cpus', '--rankfile', filename])
            if os.getenv('GEOPM_DEBUG'):
                ret.extend(['--report-bindings'])

        return ret

    def node_list_option(self):
        """
        Returns a list containing the --host option for mpiexiec.
        """
        result = []
        if self.node_list:
            nodes = self.node_list.split(',')
            node_list = []
            for node in nodes:
                if node.find(':') != -1:
                    raise SyntaxError('When explicitly specifying node list do ' +
                                      'not use ":" notation to specify number of processes')
                node_list.append(node + ':' + str(self.rank_per_node))
            result = ['--host', ','.join(node_list)]
        if self.host_file:
            result = ['--hostfile', self.host_file]
        return result

    def node_list_delim(self):
        """
        Returns the delimiter that is to be used when constructing
        a node list string given a list of node names.
        """
        return ','

class IMPIExecLauncher(Launcher):
    """
    Launcher derived object for use with the Intel(R) MPI Library job launch
    application mpiexec.hydra.
    """
    _is_once = True

    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, exclude_list=None, host_file=None,
                 reservation=None, quiet=None, do_affinity=None):
        """
        Pass through to Launcher constructor.
        """
        super(IMPIExecLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                               time_limit, job_name, node_list, exclude_list, host_file,
                                               reservation=reservation, quiet=quiet, do_affinity=do_affinity)

        self.is_slurm_enabled = False
        if os.getenv('SLURM_NNODES'):
            self.is_slurm_enabled = True
        if (self.is_geopm_enabled and
            self.is_slurm_enabled and
            self.config.get_ctl() == 'application' and
            os.getenv('SLURM_NNODES') != str(self.num_node)):
            raise RuntimeError('<geopm> geopmpy.launcher: When using srun and specifying --geopm-ctl=application call must be made ' +
                               'inside of an salloc or sbatch environment and application must run on all allocated nodes.')

    def launcher_command(self):
        """
        Returns 'mpiexec.hydra', the name of the Intel MPI Library job launch application.
        """
        return 'mpiexec.hydra'

    def parse_launcher_argv(self):
        """
        Parse the subset of srun command line arguments used or
        manipulated by GEOPM.
        """
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('-n', dest='num_rank', type=int)
        parser.add_argument('-hosts', dest='node_list', type=str)
        parser.add_argument('-f', '--hostfile', dest='host_file', type=str)
        parser.add_argument('-ppn', dest='rank_per_node', type=int)

        opts, self.argv_unparsed = parser.parse_known_args(self.argv_unparsed)
        try:
            self.argv_unparsed.remove('--')
        except ValueError:
            pass

        if opts.num_rank:
            self.num_rank = opts.num_rank
        else:
            self.num_rank = 1
        if opts.rank_per_node:
            self.rank_per_node = opts.rank_per_node
        self.node_list = opts.node_list
        self.host_file = opts.host_file
        self.cpu_per_rank = None
        self.timeout = None
        self.time_limit = None
        self.job_name = None
        self.reservation = None

    def num_node_option(self):
        return ['-ppn', str(self.rank_per_node)]

    def affinity_option(self, is_geopmctl):
        if self.is_geopm_enabled:
            # Disable other affinity mechanisms
            self.environ_ext['KMP_AFFINITY'] = 'disabled'
            self.environ_ext['MV2_ENABLE_AFFINITY'] = '0'
            self.environ_ext['KMP_WARNINGS'] = 'FALSE'

            aff_list = self.affinity_list(is_geopmctl)
            mask_zero = ['0' for ii in range(self.num_linux_cpu)]
            mask_list = []
            for cpu_set in aff_list:
                mask = list(mask_zero)
                for cpu in cpu_set:
                    mask[self.num_linux_cpu - 1 - cpu] = '1'
                # Note: Intel MPI has a bug that does not allow prefixing hex masks with 0x
                mask = '{:x}'.format(int(''.join(mask), 2))
                mask_list.append(mask)
            self.environ_ext['I_MPI_PIN_DOMAIN'] = '[{}]'.format(','.join(mask_list))
        return []

    def timeout_option(self):
        return []

    def time_limit_option(self):
        return []

    def job_name_option(self):
        return []

    def node_list_option(self):
        """
        Returns a list containing the -w option for srun.
        """
        if (self.node_list is not None and
            self.host_file is not None and
            self.node_list != self.host_file):
            raise SyntaxError('<geopm> geopmpy.launcher: Node list and host name cannot both be specified.')

        result = []
        if self.node_list is not None:
            result = ['-hosts', self.node_list]
        elif self.host_file is not None:
            result = ['-f', self.host_file]
        else:
            # If this error is encountered, without the is_once check it will be displayed 3 times per run:
            #     1. For the PlatformTopo cache creation.
            #     2. For the call to 'lscpu --hex' used in the Launcher itself.
            #     3. For actually running the app requested.
            if IMPIExecLauncher._is_once:
                sys.stderr.write('Warning: <geopm> geopmpy.launcher: Hosts not defined, GEOPM may fail to start.  '
                                 'Use "-f <host_file>" or "-hosts" to specify the hostnames of the compute nodes.\n')
                IMPIExecLauncher._is_once = False

        # If the user specified the bootstrap option, assume they know better than we do
        if not any(aa.startswith('-bootstrap') for aa in self.argv):
            if self.is_slurm_enabled:
                server = 'slurm'
                if os.getenv('SLURM_CLUSTER_NAME') == 'endeavour':
                    server = 'ssh'
                result += ['-bootstrap', server]

        return result

    def exclude_list_option(self):
        return []

    def get_idle_nodes(self):
        """
        Returns a list of the names of compute nodes that are currently
        available to run jobs using the sinfo command.
        """
        if self.is_slurm_enabled:
            return subprocess.check_output('sinfo -t idle -hNo %N | uniq', shell=True).decode().splitlines()
        else:
            raise NotImplementedError('<geopm> geopmpy.launcher: Idle nodes feature requires use with SLURM')


class AprunLauncher(Launcher):
    def __init__(self, argv, num_rank=None, num_node=None, cpu_per_rank=None, timeout=None,
                 time_limit=None, job_name=None, node_list=None, exclude_list=None, host_file=None,
                 reservation=None, quiet=None, do_affinity=None):
        """
        Pass through to Launcher constructor.
        """
        super(AprunLauncher, self).__init__(argv, num_rank, num_node, cpu_per_rank, timeout,
                                            time_limit, job_name, node_list, exclude_list, host_file,
                                            reservation=reservation, quiet=quiet, do_affinity=do_affinity)

        if self.is_geopm_enabled and self.config.get_ctl() == 'application':
            raise RuntimeError('<geopm> geopmpy.launcher: When using aprun specifying --geopm-ctl=application is not allowed.')

    def parse_launcher_argv(self):
        """
        Parse the subset of aprun command line arguments used or
        manipulated by GEOPM.
        """
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('-n', '--pes', dest='num_rank', type=int)
        parser.add_argument('-N', '--pes-per-node', dest='rank_per_node', type=int)
        parser.add_argument('-d', '--cpus-per-pe', dest='cpu_per_rank', type=int)
        parser.add_argument('-t', '--cpu-time-limit', dest='time_limit', type=int)
        parser.add_argument('-L', '--node-list', dest='node_list', type=str)
        parser.add_argument('-l', '--node-list-file', dest='host_file', type=str)
        parser.add_argument('-E', '--exclude-node-list', dest='exclude_list', type=str)

        opts, self.argv_unparsed = parser.parse_known_args(self.argv_unparsed)

        self.num_rank = opts.num_rank
        self.rank_per_node = opts.rank_per_node
        if self.num_rank is not None and self.rank_per_node is not None:
            self.num_node = int_ceil_div(self.num_rank, self.rank_per_node)
        self.cpu_per_rank = opts.cpu_per_rank
        self.timeout = None
        self.time_limit = opts.time_limit
        # Alps time limit is Linux CPU time per process.  Normalize by
        # dividing by CPU per process.
        if self.time_limit is not None and self.cpu_per_rank:
            self.time_limit = int_ceil_div(self.time_limit, self.cpu_per_rank)
        self.node_list = opts.node_list
        self.host_file = opts.host_file
        self.exclude_list = opts.exclude_list

        if (self.is_geopm_enabled and
            self.config.do_affinity and
            any(aa.startswith('--cpu-binding') or
            aa.startswith('-cc') for aa in self.argv)):
            raise SyntaxError('<geopm> geopmpy.launcher: The options --cpu-binding or -cc must not be specified, this is controlled by geopmpy.launcher.')

    def launcher_command(self):
        """
        Returns 'aprun', the name of the ALPS MPI job launch application.
        """
        return 'aprun'

    def num_node_option(self):
        """
        Returns a list containing the -N option for aprun.  Must be
        combined with the -n option to determine the number of nodes.
        """
        result = []
        if self.num_rank is not None and self.num_node is not None:
            result = ['-N', str(int_ceil_div(self.num_rank, self.num_node))]
        return result

    def affinity_option(self, is_geopmctl):
        """
        Returns the --cpu-binding option for aprun.
        """
        result = []
        if self.is_geopm_enabled:
            # Disable other affinity mechanisms
            self.environ_ext['KMP_AFFINITY'] = 'disabled'
            self.environ_ext['MV2_ENABLE_AFFINITY'] = '0'
            self.environ_ext['KMP_WARNINGS'] = 'FALSE'

            result.append('--cpu-binding')
            aff_list = self.affinity_list(is_geopmctl)
            mask_list = [range_str(cpu_set) for cpu_set in aff_list]
            result.append(':'.join(mask_list))
        return result

    def time_limit_option(self):
        """
        Returns a list containing the -t option for aprun.
        """
        result = []
        if self.time_limit is not None:
            # Mulitply by Linux CPU per process due to aprun
            # definition of time limit.
            result = ['-t', str(self.time_limit * self.cpu_per_rank)]
        return result

    def node_list_option(self):
        """
        Returns a list containing the -L option for aprun.
        """
        result = []
        if self.node_list is not None:
            result = ['-L', self.node_list]
        return result

    def host_file_option(self):
        """
        Returns a list containing the -l option for aprun.
        """
        result = []
        if self.host_file is not None:
            result = ['-l', self.host_file]
        return result

    def exclude_list_option(self):
        """
        Returns a list containing the -E option for aprun.
        """
        result = []
        if self.exclude_list is not None:
            result = ['-E', self.exclude_list]
        return result

    def preload_option(self):
        result = []
        if self.config and self.config.get_preload():
            value = ':'.join((ll for ll in
                              ('libgeopm.so', os.getenv('LD_PRELOAD'))
                              if ll is not None))
            result = ['-e',  "LD_PRELOAD='{}'".format(value)]
        return result


def main():
    """
    Main routine used by geopmlaunch wrapper executable.
    This function creates a launcher from the factory and
    calls the run method.  If help was requested on the command line
    then help from the underlying application launcher is printed and
    the help for the GEOPM extensions are appended.  Returns -1 and
    prints an error message if an error occurs.  If the GEOPM_DEBUG
    environment variable is set and an error occurs a complete stack
    trace will be printed.

    """
    err = 0
    version_str = """\
GEOPM version {}
Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.
""".format(__version__)

    launcher_prefix = "Possible LAUNCHER values:      "
    wrapper = textwrap.TextWrapper(width=80, initial_indent=launcher_prefix,
                                   subsequent_indent=' '*len(launcher_prefix))
    launchers = ', '.join('"' + ii + '"' for ii in Factory().get_launcher_names())
    help_str = """\
Usage:
      geopmlaunch LAUNCHER [GEOPM_OPTIONS] [LAUNCHER_ARGS]

GEOPM_OPTIONS:
      --geopm-report=path      create geopm report files with base name "path"
                               (default: "geopm.report")
      --geopm-report-signals=signals
                               comma-separated list of signals to add to report
      --geopm-trace=path       create geopm trace files with base name "path"
      --geopm-trace-profile=path
                               create geopm profile trace files with base name
                               "path"
      --geopm-trace-endpoint-policy=path
                               create geopm endpoint policy trace files with
                               base name "path"
      --geopm-trace-signals=signals
                               comma-separated list of signals to add as columns
                               in the trace
      --geopm-profile=name     set the name of the profile in the report and
                               trace to "name"
      --geopm-ctl=ctl          use geopm runtime and launch geopm with the
                               "ctl" method, one of "process", "pthread" or
                               "application" (default: "process")
      --geopm-agent=agent      specify the agent to be used
      --geopm-policy=pol       use the geopm policy file or shared memory
                               region "pol"
      --geopm-endpoint=endpoint
                               use shared memory keys for endpoint starting with
                               "key"
      --geopm-shmkey=key       use shared memory keys for geopm profile interaction
                               starting with "key"
      --geopm-timeout=sec      application waits "sec" seconds for handshake
                               with geopm
      --geopm-plugin-path=path look for geopm plugins in "path", a : separated
                               list of directories
      --geopm-debug-attach=rk  attach serial debugger to rank "rk"
      --geopm-record-filter=filter
                               apply the "filter" to the application record stream
      --geopm-hyperthreads-disable
                               do not allow pinning to HTs
      --geopm-ctl-disable      do not launch geopm; pass through commands to
                               underlying launcher
      --geopm-ompt-disable     disable automatic OpenMP region detection
      --geopm-affinity-disable do not emit CPU affinity settings

{}

""".format(wrapper.fill(launchers))

    try:
        # Print geopm help or version if it appears that documentation was requested
        is_help_request = ('--help' in sys.argv)
        is_version_request = ('--version' in sys.argv)
        if is_help_request or is_version_request:
            sys.argv.append('--geopm-ctl-disable')
        if sys.argv[1] not in ['--help', '--version']:
            launcher = Factory().create(sys.argv)
            launcher.run()
        if is_help_request:
            sys.stdout.write(help_str)
        if is_version_request:
            sys.stdout.write(version_str)
    except Exception as e:
        # If GEOPM_DEBUG environment variable is defined print stack trace.
        if os.getenv('GEOPM_DEBUG'):
            raise
        sys.stderr.write("<geopmpy.launcher> {err}\n".format(err=e))
        err = -1
    return err


if __name__ == '__main__':
    err = main()
    sys.exit(err)
