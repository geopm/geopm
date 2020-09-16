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

    def setup(self, run_id):
        return ''

    def cleanup(self):
        return ''

    def get_exec_path(self):
        return os.path.join(self._nasft_path, 'nas_ft')

    def get_exec_args(self):
        return []

    def get_custom_geopm_args(self):
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
