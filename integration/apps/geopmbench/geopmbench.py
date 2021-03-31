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

from .. import apps
import geopmpy.io


class DgemmAppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'DGEMM'

    def __init__(self):
        self._bench_conf = geopmpy.io.BenchConf('dgemm.conf')
        self._bench_conf.append_region('dgemm', 8.0)
        self._bench_conf.set_loop_count(500)

    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine?
        return 2

    def get_bash_setup_commands(self):
        # TODO: get rid of side effects
        self._bench_conf.write()
        return ''

    def get_bash_exec_path(self):
        # TODO: may need to find local version if not installed
        return 'geopmbench'

    def get_bash_exec_args(self):
        return self._bench_conf.get_path()


# TODO: repeated code with above
class TinyAppConf(apps.AppConf):
    ''' A smaller DGEMM for faster turnaround when testing experiment scripts.'''
    @staticmethod
    def name():
        return 'tiny'

    def __init__(self):
        self._bench_conf = geopmpy.io.BenchConf('tiny.conf')
        self._bench_conf.append_region('dgemm', 0.2)
        self._bench_conf.set_loop_count(500)

    def get_bash_setup_commands(self):
        # TODO: get rid of side effects
        self._bench_conf.write()
        return ''

    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine?
        return 2

    def get_bash_exec_path(self):
        # TODO: may need to find local version if not installed
        return 'geopmbench'

    def get_bash_exec_args(self):
        return self._bench_conf.get_path()
