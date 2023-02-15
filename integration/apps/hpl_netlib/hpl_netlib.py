#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
AppConf class for HPL reference (netlib) benchmark.
'''

import os
import sys
import math
import textwrap

from apps import apps


def setup_run_args(parser):
    """ Add common arguments for all run scripts:
        --frac-dram
    """
    help_text = 'Ratio of the total node DRAM that should be used for the HPL ' + \
                'matrix (assuming DP). Value should be between 0 and 1. ' + \
                'Default is 0.7. 0.8-0.9 is a better value but might fail due to ' + \
                'out-of-memory.'
    parser.add_argument('--frac-dram', dest='frac_dram_per_node',
                        action='store', type=float, default=0.7,
                        help=help_text)


class HplNetlibAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'hpl_netlib'

    def __init__(self, num_nodes, mach, frac_dram_per_node, cores_per_node=None):
        '''
        num_nodes: Number of MPI ranks (1 node per rank) -- 2, 4, 8 or 16.
        frac_dram_per_node: Ratio of the total node DRAM that should be used for the
                            HPL matrix (assuming DP).
                            80-90% is a good amount to maximize efficiency.
        cores_per_node: Number of Xeon cores that each MPI process can offload to via OMP.
                        Total number of physical cores will be selected if this is set to None.
        '''
        dram_for_app = num_nodes * mach.total_node_memory_bytes() * frac_dram_per_node
        if cores_per_node is None:
            cores_per_node = mach.num_core()

        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self.exec_path = os.path.join(benchmark_dir, 'hpl-2.3/bin/Linux_Intel64/xhpl')

        self.NBs = 384  # This is the recommended size for Intel (R) Xeon (R) Scalable family.
        process_grid_ratios = {
            1: {'P': 1, 'Q': 1},
            2: {'P': 1, 'Q': 2},
            4: {'P': 2, 'Q': 2},
            8: {'P': 2, 'Q': 4},
            16: {'P': 4, 'Q': 4}
        }
        if num_nodes not in process_grid_ratios:
            raise RuntimeError("Number of nodes {} is not defined for HPL.".format(num_nodes))
        self.P = process_grid_ratios[num_nodes]['P']
        self.Q = process_grid_ratios[num_nodes]['Q']

        self.N = int(round(math.sqrt(dram_for_app / 8)))
        self._cpu_per_rank = cores_per_node

        sys.stdout.write('DRAM reserved for APP: {dram_for_app:0.2f}GB\n'.format(dram_for_app=dram_for_app/2**30))
        sys.stdout.write('Cores for app: {}\n'.format(cores_per_node))
        sys.stdout.write('N={}\n'.format(self.N))

    def trial_setup(self, run_id, output_dir):
        dat_file_path = os.path.join(output_dir + "/HPL.dat")
        if not os.path.isfile(dat_file_path):
            dat_file_text = textwrap.dedent('''\
            HPLinpack benchmark input file
            Innovative Computing Laboratory, University of Tennessee
            HPL.out      output file name (if any)
            6            device out (6=stdout,7=stderr,file)
            1            # of problems sizes (N)
            {N}          Ns
            1            # of NBs
            {NBs}        NBs
            0            PMAP process mapping (0=Row-,1=Column-major)
            1            # of process grids (P x Q)
            {P}          Ps
            {Q}          Qs
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
            '''.format(N=self.N, NBs=self.NBs, P=self.P, Q=self.Q))
            with open(dat_file_path, "w") as dat_file:
                dat_file.write(dat_file_text)

    def get_rank_per_node(self):
        return 1

    def get_cpu_per_rank(self):
        return self._cpu_per_rank

    def get_bash_exec_path(self):
        return self.exec_path

    def get_bash_exec_args(self):
        return ''

    def get_custom_geopm_args(self):
        # See README.md for an explanation of why
        # HPL cannot start in process control mode.
        # Also hyperthreading does not benefit HPL and
        # it is turned off.
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
