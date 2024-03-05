#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


'''
AppConf class for NAS BT.
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
                             'rounding down to a square number and leaving one '
                             'core for GEOPM.')

def create_appconf(mach, args):
    ''' Create a NASBTAppConf object from an ArgParse and experiment.machine object.
    '''
    return NASBTAppConf(mach, args.npb_class, args.ranks_per_node, args.node_count)

class NASBTAppConf(apps.AppConf):
    def name(self):
        return f'bt.{self._npb_class}.{self._total_ranks}'

    def __init__(self, mach, npb_class, ranks_per_node, node_count):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._exec_path = os.path.join(benchmark_dir, "NPB3.4.2", "NPB3.4-MPI", "bin", "bt." + npb_class + ".x")
        self._exec_args = []
        if ranks_per_node is None:
            # Leave one core for non-app work (e.g., geopm)
            total_ranks = (mach.num_core() - 1) * node_count
        else:
            if ranks_per_node > mach.num_core():
                raise RuntimeError('The count of requested cores ({}) is more than the number of available cores ({})'.format(ranks_per_node, mach.num_core()))
            total_ranks = ranks_per_node * node_count

        # The count of NPB BT processes must be a square number
        total_ranks = math.floor(sqrt(total_ranks)) ** 2
        self._ranks_per_node = math.ceil(total_ranks / node_count)
        self._npb_class = npb_class
        self._total_ranks = total_ranks

    def get_total_ranks(self, num_nodes):
        return math.floor(sqrt(num_nodes * self._ranks_per_node)) ** 2

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_cpu_per_rank(self):
        return 1

    def get_bash_setup_commands(self):
        # The experiment launcher assumes ranks_per_node is equal across all nodes.
        # In reality, some may have one less rank than others. This means
        # ranks_per_node * node_count does not equal total_ranks. Tell NPB
        # to go ahead and round down the total rank count when this happens.
        return 'export NPB_NPROCS_STRICT=0\n'

    def parse_fom(self, log_path):
        with open(log_path) as fid:
            for line in fid.readlines():
                if line.strip().startswith('Mop/s total'):
                    total_ops_sec = float(line.split()[-1])
                    return total_ops_sec
        return float('nan')
