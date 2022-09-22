#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


'''
AppConf class for NAS EP.
'''

import os
import math

from apps import apps
from math import sqrt


def setup_run_args(parser):
    """ Add common arguments for all run scripts.
    """
    parser.add_argument('--npb-class', default='D')
    parser.add_argument('--ranks-per-node', dest='ranks_per_node',
                        action='store', type=int,
                        help='Number of physical cores to reserve for the app. '
                             'If not defined, all nodes but one will be reserved, '
                             'leaving one core for GEOPM.')

def create_appconf(mach, args):
    ''' Create a NASEPAppConf object from an ArgParse and experiment.machine object.
    '''
    return NASEPAppConf(mach, args.npb_class, args.ranks_per_node, args.node_count)

class NASEPAppConf(apps.AppConf):
    def name(self):
        return f'ep.{self._npb_class}.{self._total_ranks}'

    def __init__(self, mach, npb_class, ranks_per_node, node_count):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._exec_path = os.path.join(benchmark_dir, "NPB3.4.2", "NPB3.4-MPI", "bin", "ep." + npb_class + ".x")
        self._exec_args = []
        if ranks_per_node is None:
            # Leave one core for non-app work (e.g., geopm)
            max_cores = mach.num_core() - 1
        else:
            if ranks_per_node > mach.num_core():
                raise RuntimeError('The count of requested cores ({}) is more than the number of available cores ({})'.format(ranks_per_node, mach.num_core()))
            max_cores = ranks_per_node
        self._ranks_per_node = max_cores
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
