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

"""

import sys
import os
import optparse
import subprocess
import socket
import math

def resource_manager():
    slurm_hosts = ['mr-fusion', 'KNP12']
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

def factory(argv):
    rm = resource_manager()
    if rm == "SLURM":
        return SrunLauncher(argv[1:])
    elif rm == "ALPS":
        return AprunLauncher(argv[1:])

class PassThroughError(Exception):
    """
    Exception raised when geopm is not to be used.
    """

class SubsetOptionParser(optparse.OptionParser):
    """
    Parse a subset of comand line arguements and prepend unrecognized
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
        if '--geopm-ctl' not in argv:
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
        opts, self.args = parser.parse_args(argv)
        # Error check inputs
        if opts.policy is None:
            raise SyntaxError('--geopm-policy must be given if --geopm-ctl is specified')
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

    def __repr__(self):
        return ' '.join(['{kk}={vv}'.format(kk=kk, vv=vv)
                         for (kk, vv) in self.environ().iteritems()])

    def __str__(self):
        return self.__repr__()

    def environ(self):
        result = {}
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
        return result

    def unparsed(self):
        return self.args

class Launcher(object):
    def __init__(self, argv):
        try:
            self.config = Config(argv)
            self.argv = self.config.unparsed()
            self.parse_alloc()
        except PassThroughError:
            self.config = None
            self.argv = argv

    def run(self):
        argv_mod = self.mpiexec()
        if self.config is not None:
            argv_mod.extend(self.num_node_option())
            argv_mod.extend(self.num_rank_option())
            argv_mod.extend(self.affinity_option())
        argv_mod.extend(self.argv)
        echo = []
        if self.config:
            echo.append(self.config.__str__())
        echo.extend(argv_mod)
        echo = '\n' + ' '.join(echo) + '\n\n'
        sys.stdout.write(echo)
        sys.stdout.flush()
        subprocess.check_call(argv_mod, env=self.environ())

    def environ(self):
        result = dict(os.environ)
        if self.config is not None:
            result.update(self.config.environ())
        return result

    def parse_alloc(self):
        raise NotImplementedError('Launcher.parse_alloc() undefined in the base class')

    def mpiexec(self):
        raise NotImplementedError('Launcher.mpiexec() undefined in the base class')

    def num_node_option(self):
        raise NotImplementedError('Launcher.num_node_option() undefined in the base class')

    def num_rank_option(self):
        raise NotImplementedError('Launcher.num_rank_option() undefined in the base class')

    def affinity_option(self):
        raise NotImplementedError('Launcher.affinity_option() undefined in the base class')

class SrunLauncher(Launcher):
    def __init__(self, argv):
        super(SrunLauncher, self).__init__(argv)

    def parse_alloc(self):
        # Parse the subset of arguements used by geopm
        parser = SubsetOptionParser()
        parser.add_option('-n', '--ntasks', dest='num_rank', nargs=1, type='int')
        parser.add_option('-N', '--nodes', dest='num_node', nargs=1, type='int')
        parser.add_option('-c', '--cpus-per-task', dest='cpu_per_rank', nargs=1, type='int')
        opts, self.argv = parser.parse_args(self.argv)

        # Check required arguements
        if opts.num_rank is None:
            raise SyntaxError('Number of tasks must be specified with -n.')
        if opts.num_node is None:
            raise SyntaxError('Number of nodes must be specified with -N.')
        if any(aa.startswith('--cpu_bind') for aa in self.argv):
            raise SyntaxError('The option --cpu_bind must not be specified, this is controlled by geopm_srun.')

        self.num_rank = opts.num_rank
        self.num_app_rank = opts.num_rank
        self.num_node = opts.num_node

        if opts.cpu_per_rank is None:
            self.cpu_per_rank = int(os.environ.get('OMP_NUM_THREADS', '1'))
        else:
            self.cpu_per_rank = opts.cpu_per_rank
        if self.config.ctl == 'process':
            self.num_rank += self.num_node

    def mpiexec(self):
        return ['srun']

    def num_node_option(self):
        return ['-N', str(self.num_node)]

    def num_rank_option(self):
        return ['-n', str(self.num_rank)]

    def affinity_option(self):
        if self.config.ctl == 'application':
            raise NotImplementedError('Launch with geopmctl not supported')
        result = ['--cpu_bind']
        mask_list = []
        if self.config.ctl == 'process':
            mask_list.append('0x1')
            binary_mask = self.cpu_per_rank * '1' + '0'
        elif self.config.ctl == 'pthread':
            binary_mask = (self.cpu_per_rank + 1) * '1'
        for ii in range(self.num_app_rank):
            hex_mask = '0x{:x}'.format(int(binary_mask, 2))
            mask_list.append(hex_mask)
            if ii == 0 and self.config.ctl == 'pthread':
                binary_mask = self.cpu_per_rank * '1' + '0'
            binary_mask = binary_mask + self.cpu_per_rank * '0'
        result.append('v,mask_cpu:' + ','.join(mask_list))
        return result

class AprunLauncher(Launcher):
    def __init__(self, argv):
        super(AprunLauncher, self).__init__(argv)

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
        opts, self.argv = parser.parse_args(self.argv)

        # Check required arguements
        if opts.num_rank is None:
            raise SyntaxError('Number of tasks must be specified with -n.')
        if opts.rank_per_node is None:
            raise SyntaxError('Number of tasks per node must be specified with -N.')
        if any(aa.startswith('--cpu-binding') or
               aa.startswith('-cc') for aa in self.argv):
            raise SyntaxError('The options --cpu-binding or -cc must not be specified, this is controlled by geopm_launcher.')

        self.num_rank = opts.num_rank
        self.num_app_rank = opts.num_rank
        self.num_node = int(math.ceil(float(opts.num_rank) /
                                      float(opts.rank_per_node)))
        self.rank_per_node = opts.rank_per_node

        if opts.cpu_per_rank is None:
            self.cpu_per_rank = int(os.environ.get('OMP_NUM_THREADS', '1'))
        else:
            self.cpu_per_rank = opts.cpu_per_rank
        if self.config.ctl == 'process':
            self.num_rank += self.num_node
            self.rank_per_node += 1

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
        cpu_per_node = self.num_app_rank * self.cpu_per_rank
        if self.config.ctl == 'process':
            mask_list.append('0')
        mask_list.extend(['{0}-{1}'.format(off, off + self.cpu_per_rank - 1)
                          for off in range(off_start, cpu_per_node + off_start, self.cpu_per_rank)])
        result.append(':'.join(mask_list))
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
