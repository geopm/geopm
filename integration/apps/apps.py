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
import stat
import textwrap


class AppConf(object):
    """
    An object that contains all details needed to run an application.
    When used with experiment run scripts, setup_iteration(), get_exec_path(),
    get_exec_args(), and cleanup() will be used to construct command
    line arguments to the launcher.
    """
    @classmethod
    def name():
        return "APP"

    def __init__(self, exec_path, exec_args=[]):
        self._exec_path = exec_path
        self._exec_args = exec_args

    def get_rank_per_node(self):
        ''' Total number of ranks required by the application. '''
        return None

    def get_cpu_per_rank(self):
        ''' Hardware threads per rank required by the application. '''
        return None

    def setup(self, run_id):
        ''' Any steps to be run prior to running one iteration of the application. '''
        return ''

    def get_exec_path(self):
        ''' Application executable path. '''
        return self._exec_path

    def get_exec_args(self):
        ''' Command line arguments to the application. '''
        return self._exec_args

    def get_custom_geopm_args(self):
        ''' Additional geopm arguments required for the app, such as --geopm-ompt-disable.'''
        return []

    def cleanup(self):
        ''' Any steps to be run after running one iteration of the application. '''
        return ''

    def parse_fom(self, log_path):
        return None

    def make_bash(self, output_dir, run_id):
        # setup has side effects; call before get_exec_args
        setup = self.setup(run_id)
        app_params = self.get_exec_args()
        if type(app_params) is list:
            app_params = ' '.join(self.get_exec_args())

        script = '''#!/bin/bash\n'''
        # TODO: cd interferes with AgentConf ability to correctly write the agent policy file
        # and later be read by the controller
        script += textwrap.dedent('''\
            # cd {output_dir}
            {setup}
            {app_exec} {app_params}
            {cleanup}
        '''.format(output_dir=output_dir,
                   setup=setup,
                   app_exec=self.get_exec_path(),
                   app_params=app_params,
                   cleanup=self.cleanup()))
        bash_file = os.path.join(output_dir, '{}.sh'.format(self.name()))
        with open(bash_file, 'w') as ofile:
            ofile.write(script)
        # read, write, execute for owner
        os.chmod(bash_file, stat.S_IRWXU)
        return bash_file
