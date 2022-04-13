#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


'''
Describes the best known configuration for the NAS FT benchamrk.

'''

import os

from .. import apps


class NasftAppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'nasft'

    def __init__(self, mach):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._nasft_path = os.path.join(benchmark_dir, 'nasft/.libs')
        self._num_rank_per_node = int((mach.num_core() / 2) - 2)

    def get_rank_per_node(self):
        return self._num_rank_per_node

    def get_bash_exec_path(self):
        return os.path.join(self._nasft_path, 'nas_ft')

    def get_bash_exec_args(self):
        return []

    @staticmethod
    def parse_fom(log_path):
        result = None
        key = 'Mop/s total'
        with open(log_path) as fid:
            for line in fid:
                words = [ww.strip() for ww in line.split('=')]
                if len(words) == 2 and words[0] == key:
                    try:
                        result = float(words[1])
                    except ValueError:
                        pass
        return result
