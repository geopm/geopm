#  Copyright (c) 2015 - 2021, Intel Corporation
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
AppConf class for Arithmetic Intensity benchmark.
'''

import argparse
import os
import re
import subprocess
import sys

from apps import apps

def exec_path(run_type):
    '''
    Args

    run_type (str): One of sse, avx2 or avx512

    Returns

    Execution path for a given avx type.
    '''
    benchmark_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(benchmark_dir, "ARITHMETIC_INTENSITY", "bench_" + run_type)

def read_app_help(run_type):
    '''
    Args

    run_type (str): One of sse, avx2 or avx512

    Returns

    Help text returned by the executable of the matching avx type.
    '''
    args = [exec_path(run_type), "--help"]
    try:
        output = subprocess.run(args, check=False, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except FileNotFoundError:
        sys.stderr.write("ERROR: Executable not found: {}\n".format(exec_path(run_type)))
        sys.exit(1)
    if output.returncode != 0:
        sys.stderr.write("ERROR: App help option returned the following error:\n{}\n".format(output.stdout))
        sys.exit(1)
    # Remove the first lines up to right after "Options:"
    help_out = output.stdout.split("\n")
    start_idx = [idx for idx, val in enumerate(help_out) if re.match(r'^\s*Options:\s*$', val)][0] + 1
    return "\n".join(help_out[start_idx:])

def setup_run_args(parser):
    """ Add common arguments for all run scripts.
    """
    parser.epilog =  "Following are the additional/optional arguments passed to the app:\n\n" + read_app_help("sse")
    parser.formatter_class = argparse.RawDescriptionHelpFormatter
    parser.add_argument('--run-type', dest='run_type',
                        choices=['sse', 'avx2', 'avx512'], default='sse',
                        help='Choose a vectorization type for the run (default: sse).')
    parser.add_argument('--ranks-per-node', dest='ranks_per_node',
                        action='store', type=int,
                        help='Number of physical cores to reserve for the app. ' +
                             'If not defined, all nodes but one will be reserved (leaving one ' +
                             'node for GEOPM).')
    parser.add_argument('--distribute-slow-ranks', action='store_true',
                        help='Distribute slow ranks across nodes and packages. Otherwise, slow '
                             'ranks are assigned to fill nodes and packages (default: False).')

def create_appconf(mach, args):
    ''' Create a ArithmeticIntensityAppConf object from an ArgParse and experiment.machine object.
    '''
    return ArithmeticIntensityAppConf(mach, args.run_type, args.ranks_per_node,
                                      args.distribute_slow_ranks)

class ArithmeticIntensityAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'arithmetic_intensity'

    def __init__(self, mach, run_type, ranks_per_node, distribute_slow_ranks):
        self._exec_path = exec_path(run_type)
        self._exec_args = []
        self._distribute_slow_ranks = distribute_slow_ranks
        if ranks_per_node is None:
            self._ranks_per_node = mach.num_core() - 1
        else:
            if ranks_per_node > mach.num_core():
                raise RuntimeError('Number of requested cores is more than the number of available ' +
                                   'cores: {} vs. {}'.format(ranks_per_node, mach.num_core()))
            self._ranks_per_node = ranks_per_node

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_cpu_per_rank(self):
        return 1

    def get_custom_geopm_args(self):
        args = ['--geopm-hyperthreads-disable']
        if self._distribute_slow_ranks:
            args.extend([
                '--geopm-affinity-disable',
                '--distribution=cyclic:cyclic',
                '--ntasks-per-core=1'
            ])
        return args
