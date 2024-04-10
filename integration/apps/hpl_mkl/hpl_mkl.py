#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
AppConf class for HPL MKL benchmark.
'''

import os
import textwrap

from integration.apps.hpl_netlib import hpl_netlib


def setup_run_args(parser):
    hpl_netlib.setup_run_args(parser)

def create_appconf(mach, args):
    return HplMklAppConf(args.node_count, mach, args.frac_dram_per_node)

class HplMklAppConf(hpl_netlib.HplNetlibAppConf):
    @staticmethod
    def name():
        return 'hpl_mkl'

    def __init__(self, num_nodes, mach, frac_dram_per_node, cores_per_node=None):
        super(HplMklAppConf, self).__init__(num_nodes, mach, frac_dram_per_node, cores_per_node)
        self.mklroot = os.getenv('MKLROOT')
        self.exec_path = os.path.join(self.mklroot, 'benchmarks/mp_linpack/xhpl_intel64_dynamic')

    def get_bash_setup_commands(self):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))

        setup_commands = '{}\n'.format(os.path.join(benchmark_dir, 'check_env.sh'))
        setup_commands += 'export MKL_NUM_THREADS={}\n'.format(self._cpu_per_rank)
        setup_commands += textwrap.dedent('''
        # For Mvapich
        if [ -n "${MPIRUN_RANK}" ]; then
            PMI_RANK=${MPIRUN_RANK}
        fi

        # For OpenMPI
        if [ -n "${OMPI_COMM_WORLD_RANK}" ]; then
            PMI_RANK=${OMPI_COMM_WORLD_RANK}
        fi
        ''')
        return setup_commands
