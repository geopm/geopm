#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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

def setup_run_args(parser):
    """ Add common arguments for all run scripts.
    """
    parser.formatter_class = argparse.RawDescriptionHelpFormatter
    parser.add_argument('--run-type', dest='run_type',
                        choices=['sse', 'avx2', 'avx512'], default='sse',
                        help='Choose a vectorization type for the run (default: sse).')
    parser.add_argument('--ranks-per-node', dest='ranks_per_node',
                        action='store', type=int,
                        help='Number of physical cores to reserve for the app. '
                             'If not defined, all nodes but one will be reserved (leaving one '
                             'node for GEOPM).')
    parser.add_argument('--distribute-slow-ranks', action='store_true',
                        help='Distribute slow ranks across nodes and packages. Otherwise, slow '
                             'ranks are assigned to fill nodes and packages (default: False).')
    parser.add_argument('--slowdown', type=float, default=1,
        help='When imbalance is present, this specifies the amount of work for slow ranks'
             ' to perform, as a factor of the amount of work the fast ranks perform.')
    parser.add_argument('--base-internal-iterations', type=int,
        help='How many iterations to perform in the inner loop, for the fast set of ranks '
             '(all ranks if there is no imbalance).')
    parser.add_argument('--slow-ranks', type=int, default=0,
        help='The number of ranks to run with extra work for an imbalanced load.')
    parser.add_argument('--floats', type=int, default=67108864,
        help='The number of floating-point numbers per rank in the problem array.')
    parser.add_argument('-v', '--verbose', help='Run in verbose mode.')
    parser.add_argument('-s', '--single-precision', help='Run in single-precision mode.')
    parser.add_argument('-l', '--list', action='store_true',
        help='List the available arithmetic intensity levels for use with the --benchmarks option.')
    parser.add_argument('-i', '--iterations', type=int, default=5,
        help='The number of times to run each phase of the benchmark.')
    parser.add_argument('-b', '--benchmarks', nargs='+',
        type=float, choices=[0, 0.25, 0.5, 1, 2, 4, 8, 16, 32],
        help='List of benchmark intensity variants to run (all are run if this option is not'
             ' specified).')
    parser.add_argument('--start-time', type=int,
        help='Time at which the benchmark will start, in seconds since the system clock\'s epoch.'
             ' Start immediately by default, or if the provided time is in the past.')

def create_appconf(mach, args):
    ''' Create a ArithmeticIntensityAppConf object from an ArgParse and experiment.machine object.
    '''
    app_args = []
    for arg in ['slowdown', 'base_internal_iterations', 'slow_ranks', 'floats', 'verbose',
                'single_precision', 'list', 'iterations', 'benchmarks', 'start_time']:
        values = vars(args)[arg]
        if values is not None:
            arg = "--" + arg.replace('_', '-')
            if isinstance(values, list):
                app_args.append(arg)
                app_args += [str(ii) for ii in values]
            elif isinstance(values, bool):
                if values is True:
                    app_args.append(arg)
            else:
                app_args.append(arg)
                app_args.append(str(values))

    return ArithmeticIntensityAppConf(app_args, mach, args.run_type, args.ranks_per_node,
                                      args.distribute_slow_ranks)

class ArithmeticIntensityAppConf(apps.AppConf):
    def name(self):
        return f"arithmetic_intensity_{self.__run_type}"

    def __init__(self, app_args, mach, run_type, ranks_per_node, distribute_slow_ranks):
        self.__run_type = run_type
        self._exec_path = exec_path(run_type)
        self._exec_args = app_args
        self._distribute_slow_ranks = distribute_slow_ranks
        if ranks_per_node is None:
            self._ranks_per_node = mach.num_core() - 1
        else:
            if ranks_per_node > mach.num_core():
                raise RuntimeError('Number of requested cores is more than the number of available '
                                   'cores: {} vs. {}'.format(ranks_per_node, mach.num_core()))
            self._ranks_per_node = ranks_per_node

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_cpu_per_rank(self):
        return 1

    def get_custom_geopm_args(self):
        args = ['--geopm-hyperthreads-disable',
                '--geopm-ctl=process']
        if self._distribute_slow_ranks:
            args.extend([
                '--distribution=cyclic:cyclic',
                '--ntasks-per-core=1'
            ])
        return args
