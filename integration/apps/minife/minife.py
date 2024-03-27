#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import glob

from apps import apps


def setup_run_args(parser):
    pass

def create_appconf(mach, args):
    ''' Create a MinifeAppConf object from an ArgParse and experiment.machine object.
    '''
    return MinifeAppConf(args.node_count)

class MinifeAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'minife'

    def __init__(self, num_nodes):
        self.ranks_per_node = 2
        problem_sizes = {
            1: '-nx=396 -ny=384 -nz=384',  # '-nx=264 -ny=256 -nz=256',
            4: '-nx=528 -ny=512 -nz=512',  # '-nx=419 -ny=406 -nz=406',
            8: '-nx=792 -ny=768 -nz=768',
            64: '-nx=1056 -ny=1024 -nz=1024',
            128: '-nx=1584 -ny=1536 -nz=1536',   # '-nx=1330 -ny=1290 -nz=1290',
            256: '-nx=1676 -ny=1625 -nz=1625',
            512: '-nx=2112 -ny=2048 -nz=2048',  # scale each dimension of 1-node size by 512^(1/3)=8
        }
        if num_nodes not in problem_sizes:
            raise RuntimeError("No input size defined for minife on {} nodes".format(num_nodes))
        self.app_params = problem_sizes[num_nodes]

        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self.exe_path = os.path.join(benchmark_dir, 'miniFE_openmp-2.0-rc3/src/miniFE.x')

    def get_rank_per_node(self):
        return self.ranks_per_node

    def get_bash_exec_path(self):
        return self.exe_path

    def get_bash_exec_args(self):
        return self.app_params + ' -name=' + self.get_run_id()

    def parse_fom(self, log_path):
        # log path is ignored; use unique_name from init
        matching_files = glob.glob('*' + self.get_run_id() + '*.yaml')
        if len(matching_files) > 1:
            sys.stderr.write('<geopm> Warning: multiple yml files matched for miniFE\n')
        if len(matching_files) == 0:
            raise RuntimeError('No miniFE yml files found with pattern "{}"'.format(self.get_run_id()))

        result = ''
        # use last one in list if multiple matches
        # TODO: this is a problem if multiple trials run in parallel in the same dir
        with open(matching_files[-1]) as outfile:
            for line in outfile:
                if 'Total CG Mflops' in line:
                    result += line.split()[-1]
        return result
