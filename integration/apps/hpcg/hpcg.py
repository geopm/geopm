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
import sys
import glob
import textwrap

from .. import apps


class HpcgAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'hpcg'

    def __init__(self):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._hpcg_dir = os.path.join(benchmark_dir, 'hpcg-3.1', 'build', 'bin')
        self._ranks_per_node = 2

    def get_rank_per_node(self):
        return self._ranks_per_node

    def setup(self, run_id):
        ''' This problem should be tuned to use at least 25% of main memory.'''
        input_file = textwrap.dedent('''
        HPCG benchmark input file
        Sandia National Laboratories; University of Tennessee, Knoxville
        104 104 104
        {runtime}
        EOF
        '''.format(runtime=60))
        return 'cat > ./hpcg.dat << EOF {}'.format(input_file)

    def get_exec_path(self):
        return os.path.join(self._hpcg_dir, 'xhpcg')

    def get_exec_args(self):
        return []

    def parse_fom(self, log_path):
        # log path is ignored; look for output files in current directory
        pattern = "HPCG-Benchmark_3.1*.txt"
        matching_files = glob.glob(pattern)
        if len(matching_files) > 1:
            sys.stderr.write('<geopm> Warning: multiple output files matched for HPCG\n')
        if len(matching_files) == 0:
            raise RuntimeError('No HPCG files found with pattern "{}"'.format(pattern))

        result = ''
        # use last one in list if multiple matches
        # TODO: this is a problem
        with open(matching_files[-1]) as outfile:
            for line in outfile:
                if 'Final Summary::HPCG result is ' in line:
                    result += line.split('=')[-1]
        return result
