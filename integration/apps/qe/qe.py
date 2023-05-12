#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


''' Describes the best known configuration for Quantum Espresso.
'''

import os
import glob
import distutils.dir_util
from .. import apps

# Valid pool counts depend on how many k-points are in the input problem.
# There must be at least enough pools to distribute the k-points across them.
POOL_COUNTS_BY_NAME = {
    'qef-benchmarks/AUSURF112': 2, # 2 k-points
    'es-benchmarks/Si63Ge-scf': 4, # 4 k-points
    'es-benchmarks/Si63Ge-vc-relax': 10, # 10 k-points
    'es-benchmarks/CsI_3264nmm.cif/12': 293, # 293 k-points
    'es-benchmarks/CsI_3264nmm.cif/27': 89, # 89 k-points
    'es-benchmarks/CsI_3264nmm.cif/48': 220, # 220 k-points
    'es-benchmarks/ZrSi_6542mnm.cif/6': 352, # 352 k-points
    'es-benchmarks/ZrSi_6542mnm.cif/12': 190, # 190 k-points
    'es-benchmarks/ZrSi_6542mnm.cif/27': 92, # 92 k-points
    'es-benchmarks/ZrSi_6542mnm.cif/36': 76, # 76 k-points
    'es-benchmarks/Na2O_4242nmnm.cif/8': 1756, # 1756 k-points
    'es-benchmarks/Fe_Graphene-scf': 2, # 1 k-point. DGEMM phase fails from bad inputs with pools=1
}

def setup_run_args(parser):
    """ Add common arguments for all run scripts:
        --benchmark-name
    """
    parser.add_argument('--benchmark-name',
                        help='Specify which input to give to Quantum Espresso.',
                        choices=list(POOL_COUNTS_BY_NAME))

def create_appconf(mach, args):
    return QuantumEspressoAppConf(args.node_count)

class QuantumEspressoAppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'qe'

    def __init__(self, node_count, input_name=None):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._bin_path = os.path.join(benchmark_dir, 'q-e-qe-6.6', 'bin')
        self._node_count = node_count
        self._ranks_per_node = 20 if node_count == 1 else 10
        self._cpus_per_rank = 2 if node_count == 1 else 4

        if input_name is None:
            input_name = 'qef-benchmarks/AUSURF112'
        self._input_name = input_name.replace('/', '-')
        self._input_dir = os.path.join(benchmark_dir, input_name)

        self._pool_count = POOL_COUNTS_BY_NAME[input_name]

        total_ranks = self._node_count * self._ranks_per_node
        self._ranks_per_pool = total_ranks // self._pool_count
        while self._ranks_per_pool == 0 and self._cpus_per_rank > 1:
            print('Warning: Problem {} is too large for {} ranks. Reducing '
                  'threads per rank and increasing ranks per node.'.format(
                      input_name, total_ranks, self._pool_count))
            self._ranks_per_node = self._ranks_per_node * 2
            self._cpus_per_rank = self._cpus_per_rank // 2
            total_ranks = self._node_count * self._ranks_per_node
            self._ranks_per_pool = total_ranks // self._pool_count
            if self._ranks_per_pool == 0:
                raise ValueError('Problem {} is too large for {} ranks. Need at least {} ranks.'.format(
                input_name, total_ranks, self._pool_count));

        self._thread_group_count = total_ranks // (self._pool_count * 2)
        if self._thread_group_count % 2 == 0:
            # We perfer thread group count == ranks / (2*pools), but it needs
            # to be possible to distribute across the thread groups. So only
            # do the final division by 2 if the result is a whole number.
            self._thread_group_count = self._thread_group_count // 2
        if self._thread_group_count == 0:
            self._thread_group_count = self._ranks_per_node

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_cpu_per_rank(self):
        return self._cpus_per_rank

    def trial_setup(self, run_id, output_dir):
        # Unlike shutil, this copies the contents of the source dir, without
        # the source dir itself
        distutils.dir_util.copy_tree(self._input_dir, output_dir)
        input_files = glob.glob(os.path.join(output_dir, '*.in'))
        if len(input_files) != 1:
            raise ValueError('Expected exactly 1 *.in file present in {}. '
                             'Discovered {} files'.format(self._input_dir,
                                                          len(input_files)));
        for input_file in input_files:
            os.rename(input_file, os.path.join(output_dir, self._input_name))

    def get_bash_exec_path(self):
        return os.path.join(self._bin_path, 'pw.x')

    def get_bash_exec_args(self):
        # See https://xconfigure.readthedocs.io/en/latest/qe/ for config details
        return ['-i', self._input_name,
                '-npool', str(self._pool_count),
                # QE will internally round ndiag down to a square
                '-ndiag', str(self._ranks_per_pool),
                '-ntg', str(self._thread_group_count)]

    def get_custom_geopm_args(self):
        return ['--geopm-ctl=application',
                '--geopm-hyperthreads-disable',
                ]
