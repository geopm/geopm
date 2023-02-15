#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
AppConf class for Pennant benchmark.
'''

import os
import re

from apps import apps


def setup_run_args(parser):
    """ Add common arguments for all run scripts.
    """
    parser.add_argument('--pennant-input', dest='pennant_input',
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
    parser.add_argument('--pennant-cores-per-rank', dest='pennant_cores_per_rank',
                        action='store', type=int, default=1,
                        help='Number of physical cores to use per rank for OMP parallelism.' +
                             'Default is 1.')
    parser.add_argument('--epoch-to-outerloop', dest='epoch_to_outerloop',
                        action='store', type=int,
                        help='Chooses an appropriate pennant build with a specific GEOPM ' +
                             'epoch marker per outer loop count. If not specified, built-in '
                             'value for the problem size will be used.')
    parser.add_argument('--num-epochs', dest='num_epochs',
                        action='store', type=int,
                        help='Stop Pennant execution at the desired number of epochs (cycles in ' +
                             'Pennant terms). Has no effect if it is less than 1 or greater than the '
                             'number of cycles for the dataset.')

def create_appconf(mach, args):
    ''' Create a PennantAppConfig object from an ArgParse and experiment.machine object.
    '''
    return PennantAppConf(mach, args.pennant_input, args.pennant_cores_per_node,
                          args.pennant_cores_per_rank, args.epoch_to_outerloop, args.num_epochs)

class PennantAppConf(apps.AppConf):
    @staticmethod
    def name():
        return 'pennant'

    def __init__(self, mach, problem_file, cores_per_node, cores_per_rank, epoch_to_outerloop, num_epochs):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))

        # Based on the problem size we choose a different build of pennant.
        # The difference between these builds is how GEOPM epochs are marked:
        #    epoch100: One epoch every 100 outer loop iterations.
        #    epoch1: One epoch every outer loop iteration.
        #    default: No epoch markup.
        # There is a known issue that epochs occuring too often causes GEOPM to hang due to
        # epoch handling overwhelming GEOPM control loop. Larger problem sizes are expected to
        # handle less outer loops per epoch since outer loop time seems to go up with
        # problem size.
        exec_dir = {
            'leblancx4.pnt': 'PENNANT/build_geopm_epoch100/pennant',
            'leblancx16.pnt': 'PENNANT/build_geopm_epoch1/pennant',
            'default': 'PENNANT/build/pennant'
        }
        if epoch_to_outerloop is None:
            if os.path.basename(problem_file) in exec_dir:
                self._exec_path = os.path.join(benchmark_dir, exec_dir[os.path.basename(problem_file)])
            else:
                self._exec_path = os.path.join(benchmark_dir, exec_dir['default'])
        else:
            matching_path = [f for f in exec_dir.values() if f.find('epoch{}/'.format(epoch_to_outerloop))]
            if len(matching_path) == 1:
                self._exec_path = os.path.join(benchmark_dir, matching_path)
            else:
                raise RuntimeError('Epoch to outer loop count does not match any of the pennant builds: {}'.format(epoch_to_outerloop))

        if os.path.isfile(problem_file):
            self._exec_args = [problem_file]
        elif os.path.isfile(os.path.join(benchmark_dir, problem_file)):
            self._exec_args = [os.path.join(benchmark_dir, problem_file)]
        else:
            raise RuntimeError("Input file not found: " + problem_file)

        if not num_epochs is None:
            self._exec_args.append(str(num_epochs))

        if cores_per_node is None:
            # Reserve one for geopm.
            reserved_cores = mach.num_core() - 1
            # If odd number throw one out.
            reserved_cores = (reserved_cores // 2) * 2
            self._cores_per_node = reserved_cores
        else:
            if cores_per_node > mach.num_core():
                raise RuntimeError('Number of requested cores is more than the number ' +
                                   'of available cores: {}'.format(cores_per_node))
            self._cores_per_node = cores_per_node
        if self._cores_per_node % cores_per_rank != 0:
            raise RuntimeError('Number of requested cores is not divisible by OMP threads '
                'per rank: {} % {}'.format(self._cores_per_node, cores_per_rank))
        self._cores_per_rank = cores_per_rank


    def get_rank_per_node(self):
        return self._cores_per_node // self.get_cpu_per_rank()

    def get_cpu_per_rank(self):
        return self._cores_per_rank

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
