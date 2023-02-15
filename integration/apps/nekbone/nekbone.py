#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


'''
Describes the best known configuration for Nekbone.

On one node of mcfly, one iteration with the monitor agent takes about
300sec and produces a trace file of about 13MB.
'''

import os
import textwrap

from .. import apps


class NekboneAppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'nekbone'

    def __init__(self, add_barriers=False):
        self._add_barriers = add_barriers
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))
        self._nekbone_path = os.path.join(benchmark_dir, 'nekbone/test/example1/')
        self._num_rank_per_node = 2
        self._input_name = 'data.rea'

    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine
        return self._num_rank_per_node

    def trial_setup(self, run_id, output_dir):
        size = 10000  # this size varies per system
        input_data = textwrap.dedent('''
        .true. = ifbrick               ! brick or linear geometry
        {size} {size} 1  = iel0,ielN,ielD (per processor)  ! range of number of elements per proc.
        12 12 1 = nx0,nxN,nxD         ! poly. order range for nx1
        1  1  1 = npx, npy, npz       ! processor distribution in x,y,z
        1  1  1 = mx, my, mz          ! local element distribution in x,y,z
        '''.format(size=size))
        file_name = os.path.join(output_dir, self._input_name)
        if not os.path.exists(file_name):
            with open(file_name, 'w') as fid:
                fid.write(input_data)

    def experiment_teardown(self, output_dir):
        file_name = os.path.join(output_dir, self._input_name)
        try:
            os.unlink(file_name)
        except IOError:
            pass

    def get_bash_setup_commands(self):
        return 'ulimit -s unlimited'

    def get_bash_exec_path(self):
        binary_name = ''
        if self._add_barriers:
            binary_name = 'nekbone-barrier'
        else:
            binary_name = 'nekbone'

        return os.path.join(self._nekbone_path, binary_name)

    def get_bash_exec_args(self):
        return ['ex1']

    def get_custom_geopm_args(self):
        return ['--geopm-ompt-disable',
                '--geopm-hyperthreads-disable']

    def parse_fom(self, log_path):
        result = None
        key = 'Av MFlops'
        with open(log_path) as fid:
            for line in fid.readlines():
                if key in line:
                    result = float(line.split('=')[-1])
                    break
        return result
