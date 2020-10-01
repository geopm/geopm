#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
