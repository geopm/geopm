#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
AppConf class for NAS CG.
'''

import os

from apps import apps
import math


def setup_run_args(parser):
    """ Add common arguments for all run scripts.
    """
    parser.add_argument('--npb-class', default='D')
    parser.add_argument('--ranks-per-node', dest='ranks_per_node',
                        action='store', type=int,
                        help='Number of physical cores to reserve for the app.')

def create_appconf(mach, args):
    return NASCGAppConf(mach, args.npb_class, args.ranks_per_node, args.node_count)

class NASCGAppConf(apps.AppConf):
    def name(self):
        return f'cg.{self._npb_class}.{self._total_ranks}'

    def __init__(self, mach, npb_class, ranks_per_node, node_count):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._exec_path = os.path.join(benchmark_dir, "NPB3.4.2", "NPB3.4-MPI", "bin", "cg." + npb_class + ".x")
        self._exec_args = []
        if ranks_per_node is None:
            # Get the largest power of 2 up to the core count
            self._ranks_per_node = 2 ** math.floor(math.log2(mach.num_core()))
        else:
            if ranks_per_node > mach.num_core():
                raise RuntimeError('Number of requested cores is more than the number of available ' +
                                   'cores: {} vs. {}'.format(ranks_per_node, mach.num_core()))
            elif 2 ** math.floor(math.log2(ranks_per_node)) != ranks_per_node:
                raise RuntimeError('Number of requested cores must be a power of 2.')
            self._ranks_per_node = ranks_per_node
        self._npb_class = npb_class
        self._total_ranks = 2 ** math.floor(math.log2(node_count * self._ranks_per_node))

    def get_total_ranks(self, num_nodes):
        return self._total_ranks

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_cpu_per_rank(self):
        return 1

    def parse_fom(self, log_path):
        with open(log_path) as fid:
            for line in fid.readlines():
                if line.strip().startswith('Mop/s total'):
                    total_ops_sec = float(line.split()[-1])
                    return total_ops_sec
        return float('nan')
