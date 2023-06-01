#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import glob

from apps import apps


class CosmicTaggerAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'CosmicTagger'

    def __init__(self, mach, node_count):
        self.ranks_per_node = mach.num_gpu()

        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self.data_directory = os.path.join(benchmark_dir, 'CosmicTagger/example_data/') #TODO: Make argument

        problem_sizes = {
            1: 'mode=train run.distributed=true run.minibatch_size={} data.data_directory={}'.format(self.ranks_per_node, self.data_directory)
        }

        if node_count not in problem_sizes:
            raise RuntimeError("No input size defined for CosmicTagger on {} nodes".format(node_count))
        self.app_params = problem_sizes[node_count]

        self.exe_path = os.path.join(benchmark_dir, 'CosmicTagger/bin/exec.py')

    def get_rank_per_node(self):
        return self.ranks_per_node

    def get_bash_exec_path(self):
        return "python3 " + self.exe_path

    def get_bash_exec_args(self):
        return self.app_params + ' run.id=' + self.get_run_id()

    def parse_fom(self, log_path):
        # log path is ignored; use unique_name from init
        result = 0
        return result
