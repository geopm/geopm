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
import subprocess
import stat

from .. import apps


# TODO: this comes from baseline experiment
def get_available_app_cores(mach, pin_config):
    app_cores = mach.num_core()
    if pin_config == 'all_cores':
        pass
    elif pin_config == 'geopm_os_shared':
        app_cores -= 1
    elif pin_config == 'geopm_os_reserved':
        app_cores -= 2
    else:
        raise RuntimeError("Unknown pin_config: {}".format(pin_config))
    return app_cores


class OpenfoamAppConf(apps.AppConf):
    def __init__(self, num_nodes, mach, pin_config):
        self._benchmark_dir = os.path.dirname(os.path.abspath(__file__))

        app_cores = get_available_app_cores(mach, pin_config)

        self._num_nodes = num_nodes
        self._ranks_per_node = app_cores
        self._cpu_per_rank = 1
        self._total_ranks = self._ranks_per_node * self._num_nodes

        # 42M cell: 130 52 52
        self.NX = 130
        self.NY = 52
        self.NZ = 52

        self._mesh_result_dir = 'motorbike_mesh_{}_{}_{}'.format(self.NX, self.NY, self.NZ)
        self._setup_result_dir = 'motorbike_{}ranks_{}_{}_{}'.format(self._total_ranks, self.NX, self.NY, self.NZ)

    # TODO: temporary workaround to distinguish baseline runs
    def name(self):
        return 'openfoam_'+str(self._total_ranks)+'R'

    def get_cpu_per_rank(self):
        return self._cpu_per_rank

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_custom_geopm_args(self):
        return ['--geopm-ctl=application']

    def _environment_bash(self):
        script = ""
        # OpenFOAM bash settings
        #script += 'module purge; module load intel impi\n'
        script += 'export MPI_ROOT=$(which mpiicc | grep -o ".*/^Ci/")\n'
        script += 'export OPENFOAM_APP_DIR={}\n'.format(self._benchmark_dir)
        script += 'source ${OPENFOAM_APP_DIR}/OpenFOAM-v2006/etc/bashrc || true\n'
        return script

    def trial_setup(self, run_id, output_dir):
        start_dir = os.getcwd()
        os.chdir(output_dir)

        setup = '#!/bin/bash\n'
        setup += self._environment_bash()
        setup += 'export I_MPI_PIN_PROCESSOR=allcores:map=spread\n'
        # problem size
        setup += 'export NX={}\n'.format(self.NX)
        setup += 'export NY={}\n'.format(self.NY)
        setup += 'export NZ={}\n'.format(self.NZ)
        # Prepare input
        setup += 'export NODES={}\n'.format(self._num_nodes)
        setup += 'export NPROCS={}\n'.format(self._total_ranks)
        setup += 'export MESH_RESULT_DIR={}\n'.format(self._mesh_result_dir)
        setup += 'export SETUP_RESULT_DIR={}\n'.format(self._setup_result_dir)

        setup += textwrap.dedent('''\
        if [ -d ${OPENFOAM_APP_DIR}/${SETUP_RESULT_DIR} ]; then
            echo "Found existing decomposed mesh in $SETUP_RESULT_DIR "
        else
            echo "No decomposition for $NX $NY $NZ and ${NPROCS} ranks"
            if [ -d ${OPENFOAM_APP_DIR}/${MESH_RESULT_DIR} ]; then
                echo "Found existing mesh in $MESH_RESULT_DIR"
            else
                echo "No mesh for $NX $NY $NZ"
                cd ${OPENFOAM_APP_DIR}
                # this should have been checked out by build.sh
                cp -r ${OPENFOAM_APP_DIR}/OpenFOAM-Intel/benchmarks/motorbike/ ${MESH_RESULT_DIR}
                cd ${MESH_RESULT_DIR}

                ./Mesh $NX $NY $NZ
            fi

            cd ${OPENFOAM_APP_DIR}
            cp -r ${OPENFOAM_APP_DIR}/${MESH_RESULT_DIR}/ ${SETUP_RESULT_DIR}
            cd ${SETUP_RESULT_DIR}

            ./Setup ${NPROCS} ${NODES}
        fi
        ''')

        # TODO: try ln instead of cp
        setup += 'cd {}\n'.format(output_dir)
        setup += 'cp -rf {}/{}/* .\n'.format(self._benchmark_dir,
                                             self._setup_result_dir)

        script_name = 'prepare_input.sh'
        with open(script_name, 'w') as setup_script:
            setup_script.write(setup)
        os.chmod(script_name, stat.S_IRWXU)

        # TODO: subprocess.run is py3 only
        proc = subprocess.run('./'+script_name, shell=True)

        # return to start
        os.chdir(start_dir)

    def get_bash_setup_commands(self):
        setup = ''
        # MPI settings
        # TODO: make sure FI_PROVIDER is set in system-specific env
        setup += 'export I_MPI_FABRICS=shm:ofi\n'
        #setup += 'export FI_PROVIDER=psm2\n'
        setup += self._environment_bash()
        setup += 'source $WM_PROJECT_DIR/bin/tools/RunFunctions\n'
        return setup

    def get_bash_exec_path(self):
        return 'simpleFoam'

    def get_bash_exec_args(self):
        return '-parallel'

    def trial_teardown(self, run_id, output_dir):
        start_dir = os.getcwd()
        os.chdir(output_dir)

        # TODO: helper to make and execute bash scripts
        teardown = '#!/bin/bash\n'
        teardown += self._environment_bash()
        teardown += './Clean\n'
        script_name = 'clean_input.sh'
        with open(script_name, 'w') as script:
            script.write(teardown)
        os.chmod(script_name, stat.S_IRWXU)
        proc = subprocess.run('./'+script_name, shell=True)

        # return to start
        os.chdir(start_dir)

    def parse_fom(self, log_path):
        result = None
        pattern = "GFLOP/s rating of"
        with open(log_path) as fid:
            for line in fid.readlines():
                if pattern in line:
                    result = float(line.split()[-1])
                    break
        return result
