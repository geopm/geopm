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

    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine
        return self._num_rank_per_node

    def setup(self, run_id):
        size = 10000  # this size varies per system
        input_file = textwrap.dedent('''
        .true. = ifbrick               ! brick or linear geometry
        {size} {size} 1  = iel0,ielN,ielD (per processor)  ! range of number of elements per proc.
        12 12 1 = nx0,nxN,nxD         ! poly. order range for nx1
        1  1  1 = npx, npy, npz       ! processor distribution in x,y,z
        1  1  1 = mx, my, mz          ! local element distribution in x,y,z
        EOF
        '''.format(size=size))
        return 'ulimit -s unlimited; cat > ./data.rea << EOF {}'.format(input_file)

    def cleanup(self):
        return 'rm -f ./data.rea'

    def get_exec_path(self):
        binary_name = ''
        if self._add_barriers:
            binary_name = 'nekbone-barrier'
        else:
            binary_name = 'nekbone'

        return os.path.join(self._nekbone_path, binary_name)

    def get_exec_args(self):
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
