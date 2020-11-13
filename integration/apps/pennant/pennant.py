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
AppConf class for Pennant benchmark.
'''

import os
import re

from apps import apps


def setup_run_args(parser):
    """ Add common arguments for all run scripts:
        --input
    """
    parser.add_argument('--pennant-input', dest='pennant_input'
                        action='store', type=str,
                        default="PENNANT/test/leblancx4/leblancx4.pnt",
                        help='Path to the input file (see .pnt files in test' +
                             'directory of the PENNANT source tarball). ' +
                             'Absolute path or relative to the app ' +
                             'directory. Default is leblancx4.pnt')
    parser.add_argument('--pennant-cores-per-node', dest='pennant_cores_per_node',
                        action='store', type=int,
                        help='Number of physical cores to reserve for the app. ' +
                             'If not defined, highest even number of cores less than ' +
                             'the total number in the node will be reserved (leaving at ' +
                             'least one node for GEOPM).')


class PennantAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'pennant'

    def __init__(self, mach, problem_file="PENNANT/test/leblancx4/leblancx4.pnt", cores_per_node=None):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._exec_path = os.path.join(benchmark_dir, 'PENNANT/build/pennant')
        if os.path.isfile(problem_file):
            self._exec_args = problem_file
        elif os.path.isfile(os.path.join(benchmark_dir, problem_file)):
            self._exec_args = os.path.join(benchmark_dir, problem_file)
        else:
            raise RuntimeError("Input file not found: " + problem_file)
        if cores_per_node is None:
            # Reserve one for geopm.
            reserved_cores = mach.num_core() - 1
            # If odd number throw one out.
            reserved_cores = (reserved_cores // 2) * 2
            self._cores_per_node = reserved_cores
        else:
            self._cores_per_node = cores_per_node


    def get_rank_per_node(self):
        return self._cores_per_node // self.get_cpu_per_rank()

    def get_cpu_per_rank(self):
        return 1

    def get_custom_geopm_args(self):
        return ['--geopm-hyperthreads-disable', '--geopm-ompt-disable']

    def parse_fom(self, log_path):
        with open(log_path) as fid:
            for line in fid.readlines():
                float_regex = r'([-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?)'
                m_zones = re.match(r'^Zones:\s*' + float_regex, line)
                m_runtime = re.match(r'^hydro cycle run time=\s*' +
                                     float_regex, line)
                m_cycle = re.match(r'^cycle\s*=\s*' + float_regex, line)
                if m_zones:
                    zones = int(m_zones.group(1))
                if m_runtime:
                    runtime = float(m_runtime.group(1))
                if m_cycle:
                    cycle = int(m_cycle.group(1))
        return zones * cycle / runtime
