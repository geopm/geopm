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
import textwrap

from apps.hpl_netlib import hpl_netlib


class HplMklAppConf(hpl_netlib.HplNetlibAppConf):
    @staticmethod
    def name():
        return 'hpl_mkl'

    def __init__(self, num_nodes, mach, perc_dram_per_node=0.9, cores_per_node=None):
        super().__init__(num_nodes, mach, perc_dram_per_node, cores_per_node)
        self.mklroot = os.getenv('MKLROOT')
        self.exec_path = os.path.join(self.mklroot, 'benchmarks/mp_linpack/xhpl_intel64_dynamic')

    def get_bash_setup_commands(self):
        benchmark_dir = os.path.dirname(os.path.abspath(__file__))

        setup_commands += '{}\n'.format(os.path.join(benchmark_dir, 'check_env.sh'))
        setup_commands += 'export MKL_NUM_THREADS={}\n'.format(self._cpu_per_rank)
        setup_commands += textwrap.dedent('''
        # For Mvapich
        if [ -n "${MPIRUN_RANK}" ]; then
            PMI_RANK=${MPIRUN_RANK}
        fi

        # For OpenMPI
        if [ -n "${OMPI_COMM_WORLD_RANK}" ]; then
            PMI_RANK=${OMPI_COMM_WORLD_RANK}
        fi
        ''')
        return setup_commands
