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

    def __init__(self, mach, mkl_version=False):
        self._mkl_version = mkl_version
        if self._mkl_version:
            # use Intel(R) MKL benchmark; requires Intel(R) MKL to be installed
            benchmark_dir = '/opt/ohpc/pub/compiler/intel/compilers_and_libraries_2019.4.243/linux/mkl/benchmarks/hpcg/bin'
            self._hpcg_exe = os.path.join(benchmark_dir, 'xhpcg_skx')
        else:
            # use reference version; will be slower
            benchmark_dir = os.path.dirname(os.path.abspath(__file__))
            self._hpcg_exe = os.path.join(benchmark_dir, 'hpcg-3.1', 'build', 'bin', 'xhpcg')

        self._ranks_per_node = mach.num_package()

        # TODO: should have 1 OMP thread per core

    def get_rank_per_node(self):
        return self._ranks_per_node

    def setup(self, run_id):
        # per-process size; should be tuned to use at least 25% of main memory.
        # must be multiple of 8 and greater than 24
        # size for mcfly (93GB per node) with 2 ranks per node:
        size = "256 256 256"  # 24GB, 1000 sec with reference, 220 sec with MKL benchmark
        runtime = "1800"
        result = ''
        if self._mkl_version:
            self._exec_args = '-n{} -t{}'.format(size, runtime)
        else:
            self._exec_args = ''
            input_file = textwrap.dedent('''
            HPCG benchmark input file
            GEOPM integration
            {problem}
            {runtime}
            EOF
            '''.format(runtime=runtime, problem=size))
            result = 'cat > ./hpcg.dat << EOF {}'.format(input_file)
        return result

    def get_exec_path(self):
        return self._hpcg_exe

    def get_exec_args(self):
        return self._exec_args

    def parse_fom(self, log_path):
        result = None
        if self._mkl_version:
            pattern = "GFLOP/s rating of"
            with open(log_path) as fid:
                for line in fid.readlines():
                    if pattern in line:
                        result = float(line.split()[-1])
                        break
        else:
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
