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

'''
Configuration for AMG.
'''

import os
from .. import apps


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
        self.exe_path = os.path.join(benchmark_dir, 'AMG-master/test/amg')

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
