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

import os
import sys
import glob
import textwrap
import subprocess

from apps import apps

def setup_run_args(parser):
    """ Add common arguments for all run scripts.                                                                                                                                                                                                                                                                                                                
    """
    parser.add_argument('--parres-init-setup-file', dest='parres_init_setup',
                        action='store', type=str,
                        help='Path to the parres-init-setup-file' +
                             'Absolute path or relative to the app' +
                             'directory. Default is None')
    parser.add_argument('--parres-exp-setup-file', dest='parres_exp_setup',
                        action='store', type=str,
                        help='Path to the parres-exp-setup-file' +
                             'Absolute path or relative to the app' +
                             'directory. Default is None')
    parser.add_argument('--parres-teardown-file', dest='parres_teardown',
                        action='store', type=str,
                        help='Path to the parres-teardown-file' +
                             'Absolute path or relative to the app' +
                             'directory. Default is None')
    parser.add_argument('--parres-cores-per-node', dest='parres_cores_per_node',
                        action='store', type=int,
                        help='Number of physical cores to reserve for the app. ' +
                             'If not defined, app-type will determine setting.')
    parser.add_argument('--parres-gpus-per-node', dest='parres_gpus_per_node',
                        action='store', type=int,
                        help='Number of physical gpus to reserve for the app. ' +
                             'If not defined, app-type will determine setting.')
    parser.add_argument('--parres-cores-per-rank', dest='parres_cores_per_rank',
                        action='store', type=int, default=1,
                        help='Number of physical cores to use per rank for OMP parallelism.' +
                             'Default is 1.')
    parser.add_argument('--parres-args', dest='parres_args',
                        action='store', type=str,
                        help='Arguments for parres binary')

    
    
def create_dgemm_appconf(mach, args):
    ''' Create a ParresAppConfig object from an ArgParse and experiment.machine object.                                                                                                                                                                                                                                                                         
    '''
    return ParresDgemmAppConf(mach, args.node_count, args.parres_cores_per_node, args.node_count,
                              args.parres_gpus_per_node, args.parres_cores_per_rank, args.parres_init_setup, args.parres_exp_setup, args.parres_teardown, args.parres_args)

    
class ParresDgemmAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'parres_dgemm'

    def __init__(self, mach, num_nodes, cores_per_node, node_count, gpus_per_node, cores_per_rank, parres_init_setup, parres_exp_setup, parres_teardown, parres_args):
        if cores_per_node is None:
            self._cores_per_node = 4
        else:
            if cores_per_node > mach.num_core():
                raise RuntimeError('Number of requested cores is more than the number ' +
                                   'of available cores: {}'.format(cores_per_node))
            self._cores_per_node = cores_per_node
        if self._cores_per_node % cores_per_rank != 0:
            raise RuntimeError('Number of requested cores is not divisible by OMP threads '
                               'per rank: {} % {}'.format(self._cores_per_node, cores_per_rank))
        self._cores_per_rank = cores_per_rank

        if node_count != 1:
            raise RuntimeError('ParRes Dgemm is only setup for 1 node not {}'.format(node_count))
        
        if gpus_per_node is None:
            gpus_per_node = mach.num_board_accelerator()
        else:
            if (gpus_per_node != mach.num_board_accelerator()):
                raise RuntimeError('Number of requested gpus must be the same as the available # of gpus')

        if not ( self._cores_per_node // self.get_cpu_per_rank() == gpus_per_node):           
            raise RunTimeError('Can currently only handle the same # of ranks and gpus per node')
                
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))

        if not parres_init_setup is None:
            if not os.path.isfile(parres_init_setup):
                if (os.path.isfile(os.path.join(benchmark_dir, parres_init_setup))):
                    parres_init_setup = os.path.join(benchmark_dir, parres_init_setup)
                else:
                    raise runtimeError("Parres-init-setup file not found:" + parres_init_setup)
            os.chmod(parres_init_setup, 0o755)
            subprocess.call(parres_init_setup, shell=True)

        if not parres_exp_setup is None:
            if not os.path.isfile(parres_exp_setup):
                if (os.path.isfile(os.path.join(benchmark_dir, parres_exp_setup))):
                    parres_exp_setup = os.path.join(benchmark_dir, parres_exp_setup)
                else:
                    raise runtimeError("Parres-exp-setup file not found:" + parres_exp_setup)
            
        if not parres_teardown is None:
            if not os.path.isfile(parres_teardown):
                if (os.path.isfile(os.path.join(benchmark_dir, parres_teardown))):
                    parres_teardown = os.path.join(benchmark_dir, parres_teardown)
                else:
                    raise runtimeError("Parres-teardown file not found:" + parres_teardown)
                
        self._parres_exp_setup = parres_exp_setup
        self._parres_teardown = parres_teardown

        binary_name = 'Kernels/Cxx11/dgemm-mpi-cublas'
        if parres_args is None:
            params = ["10","16000"]
        else:
            params=parres_args.split()

        self.app_params = " ".join(params)

        self.exe_path = os.path.join(benchmark_dir, binary_name)


    def get_rank_per_node(self):
        return self._cores_per_node // self.get_cpu_per_rank()

    def get_cpu_per_rank(self):
        return self._cores_per_rank

    def get_bash_exec_path(self):
        return self.exe_path

    def get_bash_exec_args(self):
        return self.app_params

    def get_bash_setup_commands(self):
        setup_commands = textwrap.dedent('''
        ulimit -s unlimited
        export I_MPI_FABRICS=shm
        ''')

        return setup_commands

    def trial_setup(self, run_id, output_dir):
        if not self._parres_exp_setup is None:
            os.chmod(self._parres_exp_setup, 0o755)
            subprocess.call(self._parres_exp_setup, shell=True)

        
    
    def experiment_teardown(self, output_dir):
        if not self._parres_teardown is None:
            os.chmod(self._parres_teardown, 0o755)
            subprocess.call(self._parres_teardown, shell=True)

    def parse_fom(self, log_path):
        result = None
        key = 'AVG Rate (MF/s): '
        with open(log_path) as fid:
            for line in fid.readlines():
                if key in line:
                    result = line.split(': ')[1]
                    result = float(result.split(' ')[0])
                    break
        return result

            
def create_nstream_appconf(mach, args):
    ''' Create a ParresAppConfig object from an ArgParse and experiment.machine object.                                                                                                                                                                                                                                                                         
    '''
    return ParresNstreamAppConf(mach, args.node_count, args.parres_cores_per_node, args.node_count,
                                args.parres_gpus_per_node, args.parres_cores_per_rank, args.parres_init_setup, args.parres_exp_setup, args.parres_teardown,
                                args.parres_args)

    
class ParresNstreamAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'parres_nstream'

    def __init__(self, mach, num_nodes, cores_per_node, node_count, gpus_per_node, cores_per_rank, parres_init_setup, parres_exp_setup, parres_teardown, parres_args):
        if cores_per_node is None:
            self._cores_per_node = 1
        else:
            if cores_per_node != 1:
                raise RuntimeError('Currently Nstream can only handle 1 core per Node')
            self._cores_per_node = cores_per_node
        if self._cores_per_node % cores_per_rank != 0:
            raise RuntimeError('Number of requested cores is not divisible by OMP threads '
                               'per rank: {} % {}'.format(self._cores_per_node, cores_per_rank))
        self._cores_per_rank = cores_per_rank

        if node_count != 1:
            raise RuntimeError('ParRes Dgemm is only setup for 1 node not {}'.format(node_count))
        
        if gpus_per_node is None:
            gpus_per_node = 1
        else:
            if (gpus_per_node > mach.num_board_accelerator()):
                raise RuntimeError('Number of requested gpus is more than the number ' +
                                   'of available gpus: {}'.format(gpus_per_node))

        if not ( ( self._cores_per_node // self.get_cpu_per_rank() == 1 ) and gpus_per_node == 1):           
            raise RunTimeError('Can currently only handle 1 ranks per node and 1 gpus per node')
                
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))

        if not parres_init_setup is None:
            if not os.path.isfile(parres_init_setup):
                if (os.path.isfile(os.path.join(benchmark_dir, parres_init_setup))):
                    parres_init_setup = os.path.join(benchmark_dir, parres_init_setup)
                else:
                    raise runtimeError("Parres-init-setup file not found:" + parres_init_setup)
            os.chmod(parres_init_setup, 0o755)
            subprocess.call(parres_init_setup, shell=True)

        if not parres_exp_setup is None:
            if not os.path.isfile(parres_exp_setup):
                if (os.path.isfile(os.path.join(benchmark_dir, parres_exp_setup))):
                    parres_exp_setup = os.path.join(benchmark_dir, parres_exp_setup)
                else:
                    raise runtimeError("Parres-exp-setup file not found:" + parres_exp_setup)
            
        if not parres_teardown is None:
            if not os.path.isfile(parres_teardown):
                if (os.path.isfile(os.path.join(benchmark_dir, parres_teardown))):
                    parres_teardown = os.path.join(benchmark_dir, parres_teardown)
                else:
                    raise runtimeError("Parres-teardown file not found:" + parres_teardown)
                
        self._parres_exp_setup = parres_exp_setup
        self._parres_teardown = parres_teardown

        binary_name = 'Kernels/Cxx11/nstream-mpi-cuda'
        if parres_args is None:
            params = ["1","1000000000"]
        else:
            params=parres_args.split()

        self.app_params = " ".join(params)

        self.exe_path = os.path.join(benchmark_dir, binary_name)


    def get_rank_per_node(self):
        return self._cores_per_node // self.get_cpu_per_rank()

    def get_cpu_per_rank(self):
        return self._cores_per_rank

    def get_bash_exec_path(self):
        return self.exe_path

    def get_bash_exec_args(self):
        return self.app_params

    def get_bash_setup_commands(self):
        setup_commands = textwrap.dedent('''
        ulimit -s unlimited
        export I_MPI_FABRICS=shm
        ''')

        return setup_commands

    def trial_setup(self, run_id, output_dir):
        if not self._parres_exp_setup is None:
            os.chmod(self._parres_exp_setup, 0o755)
            subprocess.call(self._parres_exp_setup, shell=True)

        
    
    def experiment_teardown(self, output_dir):
        if not self._parres_teardown is None:
            os.chmod(self._parres_teardown, 0o755)
            subprocess.call(self._parres_teardown, shell=True)


    def parse_fom(self, log_path):
        result = None
        key = 'Rate (MB/s): '
        with open(log_path) as fid:
            for line in fid.readlines():
                if key in line:
                    result = line.split(': ')[1]
                    result = float(result.split(' ')[0])
                    break
        return result

