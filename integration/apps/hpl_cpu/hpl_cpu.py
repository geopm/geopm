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

import os
import sys
import glob
import math
import textwrap

from apps import apps


class HplCpuAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'hpl_cpu'

    def __init__(self, num_nodes, mach, perc_dram_per_node=0.9, cores_per_node=None):
        '''
        num_nodes: Number of MPI ranks (1 node per rank) -- 2, 4, 8 or 16.
        perc_dram_per_node: Ratio of the total node DRAM that should be used for the
                            HPL matrix (assuming DP).
                            80-90% is a good amount to maximize efficiency.
                            Default is 0.9.
        cores_per_node: Number of Xeon cores that each MPI process can offload to via OMP.
                        Total number of physical cores will be selected if this is not set
                        (defauilt=None).
        '''
        dram_for_app = num_nodes * mach.total_node_memory_bytes() * perc_dram_per_node
        if cores_per_node is None:
            cores_per_node = mach.num_core()

        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self.exe_path = os.path.join(benchmark_dir, 'hpl-2.3/bin/Linux_Intel64/xhpl')
        self.NBs=384 # This is the recommended size for Intel Scalable Xeon family.
        process_grid_ratios = {
            1: {'P': 1, 'Q': 1},
            2: {'P': 1, 'Q': 2},
            4: {'P': 2, 'Q': 2},
            8: {'P': 2, 'Q': 4},
            16: {'P': 4, 'Q': 4}
        }
        if num_nodes not in process_grid_ratios:
            raise RuntimeError(f"Number of nodes {num_nodes} is not defined for HPL.")
        self.P = process_grid_ratios[num_nodes]['P']
        self.Q = process_grid_ratios[num_nodes]['Q']

        self.N = int(round(math.sqrt(dram_for_app / 8)))
        self.cores_per_node = cores_per_node

        print(f'DRAM reserved for APP: {dram_for_app} Cores for app: {cores_per_node} N={self.N}')

    def get_bash_setup_commands(self):
        input_file = textwrap.dedent(f'''
        HPLinpack benchmark input file
        Innovative Computing Laboratory, University of Tennessee
        HPL.out      output file name (if any)
        6            device out (6=stdout,7=stderr,file)
        1            # of problems sizes (N)
        {self.N}     Ns
        1            # of NBs
        {self.NBs}   NBs
        0            PMAP process mapping (0=Row-,1=Column-major)
        1            # of process grids (P x Q)
        {self.P}     Ps
        {self.Q}     Qs
        16.0         threshold
        1            # of panel fact
        1            PFACTs (0=left, 1=Crout, 2=Right)1
        1            # of recursive stopping criterium
        4            NBMINs (>= 1)
        1            # of panels in recursion
        2            NDIVs
        1            # of recursive panel fact.
        1            RFACTs (0=left, 1=Crout, 2=Right)
        1            # of broadcast
        0            BCASTs (0=1rg,1=1rM,2=2rg,3=2rM,4=Lng,5=LnM)
        1            # of lookahead depth
        0            DEPTHs (>=0)
        2            SWAP (0=bin-exch,1=long,2=mix)
        64           swapping threshold
        0            L1 in (0=transposed,1=no-transposed) form
        0            U  in (0=transposed,1=no-transposed) form
        1            Equilibration (0=no,1=yes)
        8            memory alignment in double (> 0)
        EOF
        ''')
        # KMP_HW_SUBSET: forces 1 thread per core to be used
        # OMP_NUM_THREADS: # number of cores per node
        # MKL_NUM_THREADS: # matches with above, effective if linked against MKL
        return f'export KMP_HW_SUBSET=1t; ' \
               f'export OMP_NUM_THREADS={self.cores_per_node}; ' \
               f'export MKL_NUM_THREADS={self.cores_per_node}; ' \
               f'cat > ./HPL.dat << EOF {input_file}'

    def get_rank_per_node(self):
        return 1

    def get_bash_exec_path(self):
        return self.exe_path

    def get_bash_exec_args(self):
        return ''

    def get_custom_geopm_args(self):
        return ['--geopm-ctl=application',
                '--geopm-hyperthreads-disable']

    def parse_fom(self, log_path):
        result = None
        key = 'WR00'
        with open(log_path) as fid:
            for line in fid.readlines():
                if key in line:
                    result = float(line.split(' ')[-1])
                    break
        return result
