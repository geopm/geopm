#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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

class GeopmbenchAppConf(apps.AppConf):
    ''' A version that user can pass config files.'''
    @staticmethod
    def name():
        return 'geopmbench'

    def __init__(self, bench_conf_path, ranks_per_node):
        self._bench_conf_path = bench_conf_path
        self._ranks_per_node = ranks_per_node

    def get_rank_per_node(self):
        return self._ranks_per_node

    def get_bash_exec_path(self):
        # TODO: may need to find local version if not installed
        return 'geopmbench'

    def get_bash_exec_args(self):
        return self._bench_conf_path

def setup_geopmbench_run_args(parser):
    """ Add common arguments for all run scripts.
    Note that config_file is a positional argument and is required by default.
    """
    parser.add_argument('config_file', action='store', type=str,
                        help='Path to the geopmbench config json file.')
    parser.add_argument('--ranks-per-node', dest='ranks_per_node',
                        action='store', type=int,
                        help='Number of MPI ranks to reserve for the app per node.')

def create_geopmbench_appconf(mach, args):
    ''' Create a AppConfig object from an ArgParse and experiment.machine object.
    '''
    if args.ranks_per_node is None:
        args.ranks_per_node = mach.num_package()
    return GeopmbenchAppConf(args.config_file, args.ranks_per_node)
