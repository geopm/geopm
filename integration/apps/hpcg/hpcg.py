#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os

from .. import apps


class HpcgAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'hpcg'

    def __init__(self, mach):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._hpcg_exe = os.path.join(benchmark_dir, 'hpcg', 'bin', 'xhpcg.x')

        self._ranks_per_node = mach.num_package()
        self._cpu_per_rank = (mach.num_core() - 2) // self._ranks_per_node

        # per-process size; should be tuned to use at least 25% of main memory.
        # must be multiple of 8 and greater than 24
        # size for mcfly (93GB per node) with 2 ranks per node:
        size = "256 256 256"  # 24GB, 1000 sec with reference, 220 sec with MKL benchmark
        runtime = "1800"
        self._exec_args = '-n{} -t{}'.format(size, runtime)

    def get_cpu_per_rank(self):
        return self._cpu_per_rank

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_bash_exec_path(self):
        return self._hpcg_exe

    def get_bash_exec_args(self):
        return self._exec_args

    def parse_fom(self, log_path):
        result = None
        pattern = "GFLOP/s rating of"
        with open(log_path) as fid:
            for line in fid.readlines():
                if pattern in line:
                    result = float(line.split()[-1])
                    break
        return result
