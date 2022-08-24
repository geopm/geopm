#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
AppConf class for NAS LU.
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
    return NASLUAppConf(mach, args.npb_class, args.ranks_per_node, args.node_count)

class NASLUAppConf(apps.AppConf):
    def name(self):
        return f'lu.{self._npb_class}.{self._total_ranks}'

    def __init__(self, mach, npb_class, ranks_per_node, node_count):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._exec_path = os.path.join(benchmark_dir, "NPB3.4.2", "NPB3.4-MPI", "bin", "lu." + npb_class + ".x")
        self._exec_args = []
        if ranks_per_node is None:
            # LU expects a rank count that can be distributed across a grid with
            # X and Y dimensions of similar magnitude. The following is an
            # approximation of the greatest rank count that can achieve that
            # goal, but it is not exact. Alternatively, set --ranks-per-node
            # to the core count, then run LU and check its error output to see
            # recommended rank counts.
            grid_x_values = [math.floor(math.sqrt(mach.num_core() // n)) for n in (0.5, 1, 2)]
            grid_y_values = [mach.num_core() // x for x in grid_x_values]
            self._ranks_per_node = max(x*y for x, y in zip(grid_x_values, grid_y_values) if abs(x-y) <= max(x, y) / 2)
        else:
            if ranks_per_node > mach.num_core():
                raise RuntimeError('Number of requested cores is more than the number of available ' +
                                   'cores: {} vs. {}'.format(ranks_per_node, mach.num_core()))
            self._ranks_per_node = ranks_per_node
        self._npb_class = npb_class
        self._total_ranks = self._ranks_per_node * node_count

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
