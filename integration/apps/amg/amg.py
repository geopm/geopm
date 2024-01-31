#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Configuration for AMG.
'''

import os
from .. import apps


def setup_run_args(parser):
    pass

def create_appconf(mach, args):
    return AmgAppConf(args.node_count)

class AmgAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'amg'

    def __init__(self, num_nodes):
        self.num_nodes = num_nodes
        self.ranks_per_node = 16
        problem = '-problem 1'
        size_per_proc = '-n 96 192 192'
        process_config = None
        # total product of -P must == total ranks
        total_ranks = num_nodes * self.ranks_per_node
        if total_ranks == 16:     # 1 node, 16 ranks
            process_config = '-P 4 2 2'
        elif total_ranks == 64:   # 4 nodes, 16 ranks
            process_config = '-P 4 4 4'
        elif total_ranks == 128:  # 8 nodes, 16 ranks
            process_config = '-P 8 4 4'
        elif total_ranks == 2048:   # 128 nodes, 16 ranks
            process_config = '-P 16 16 8'
        else:
            raise RuntimeError("No input size defined for amg on {} nodes with {} ranks per node".format(self.num_nodes, self.ranks_per_node))

        self.app_params = problem + ' ' + size_per_proc + ' ' + process_config
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self.exe_path = os.path.join(benchmark_dir, 'AMG/test/amg')

    def get_rank_per_node(self):
        return self.ranks_per_node

    def get_bash_exec_path(self):
        return self.exe_path

    def get_bash_exec_args(self):
        return self.app_params

    def parse_fom(self, logfile):
        result = ''
        with open(logfile) as log:
            for line in log:
                if 'Figure of Merit' in line:
                    result += line.split()[-1] + ' '
        return result
