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


'''
Describes the best known configuration for WRF.
'''

import os
import textwrap
import subprocess

from .. import apps


class WrfAppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'WRF'

    def __init__(self, mach, num_nodes):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._Wrf_path = os.path.join(benchmark_dir, 'WRFV3/run/')
        self._num_nodes = num_nodes
        reserved_cores = 2;

        if num_nodes == 1:
            self._cpu_per_rank = 1
            self._num_rank_per_node = (mach.num_core() - reserved_cores)/self._cpu_per_rank
            self._wrf_num_tiles=80
        elif num_nodes == 2:
            self._cpu_per_rank = 2
            self._num_rank_per_node = (mach.num_core() - reserved_cores)/self._cpu_per_rank
            self._wrf_num_tiles=60
        elif num_nodes == 4:
            self._cpu_per_rank = 2
            self._num_rank_per_node = (mach.num_core() - reserved_cores)/self._cpu_per_rank
            self._wrf_num_tiles=40
        elif num_nodes == 8:
            self._cpu_per_rank = 4
            self._num_rank_per_node = (mach.num_core() - reserved_cores)/self._cpu_per_rank
            self._wrf_num_tiles=20
        elif num_nodes == 16:
            self._cpu_per_rank = 4
            self._num_rank_per_node = (mach.num_core() - reserved_cores)/self._cpu_per_rank
            self._wrf_num_tiles=10
        else:
            raise RuntimeError("An unsupported number of nodes ({}) was provided.  Supported node count: 1,2,4,8,16".format(num_nodes))

    #Sets OMP Num Threads
    def get_cpu_per_rank(self):
        return self._cpu_per_rank

    #Sets ranks per node
    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine
        return self._num_rank_per_node

    def get_bash_setup_commands(self):
        return_string =  '''\
ulimit -s unlimited;
export OMP_PLACES=cores
export OMP_PROC_BIND=close
export WRFIO_NCD_LARGE_FILE_SUPPORT=1
export WRF_NUM_TILES={}
'''.format(self._wrf_num_tiles)
        if self._num_nodes == 1:
            return_string += "export OMP_STACKSIZE=128M\n"
        else:
            return_string += "export OMP_STACKSIZE=512M\n"

        return return_string

    def get_bash_exec_path(self):
        binary_name = 'wrf.exe'
        return os.path.join(self._Wrf_path, binary_name)
        #TODO: replace with print_affinity from examples folder to check pinning

    def get_bash_exec_args(self):
        return ""

    def trial_setup(self, run_id, output_dir):
        exec_str = "ln -sf {}* {}/.".format(self._Wrf_path, output_dir)
        subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, shell=True)

    def trial_teardown(self, run_id, output_dir):
        exec_str = "mv {}/rsl.out.0000 {}/rsl.out.{}".format(output_dir, output_dir, run_id)
        subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, shell=True)
        pass

    def parse_fom(self, log_path):
        key = "Timing for main"
        idx = 0;
        timestep_sum = 0;
        filename = 'rsl.out.0000'
        with open(filename) as fid:
            for line in fid.readlines():
                line = line.rstrip()  # remove '\n' at end of line
                if key in line:
                    if idx > 0: #First value skews the FOM
                        timestep_sum += float(line.split(':')[-1].strip().split()[0])
                    idx += 1;

        avg_time = 1/(timestep_sum/idx);
        return avg_time;
