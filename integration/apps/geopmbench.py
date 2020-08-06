import os
import textwrap

from . import apps
import geopmpy.io


class DgemmAppConf(apps.AppConf):

    @staticmethod
    def name():
        return 'DGEMM'

    def __init__(self, output_dir):
        self._bench_conf = geopmpy.io.BenchConf(os.path.join(output_dir, 'dgemm.conf'))
        self._bench_conf.append_region('dgemm', 8.0)
        self._bench_conf.set_loop_count(500)

    def get_num_node(self):
        # TODO: this seems bad
        # proposal: num_nodes always an input to __init__, app throws if it can't handle.
        # num ranks per node always an output from app conf.
        # if you want to play with different rank configs, use different app conf
        raise RuntimeError("<geopm> dgemm application scales to any number of nodes.")
        return None

    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine?
        return 2

    def setup(self):
        self._bench_conf.write()
        return ""

    def get_exec_path(self):
        # TODO: may need to find local version if not installed
        return 'geopmbench'

    def get_exec_args(self):
        return self._bench_conf.get_path()


# TODO: repeated code with above
class TinyAppConf(apps.AppConf):
    ''' A smaller DGEMM for faster turnaround when testing experiment scripts.'''
    @staticmethod
    def name():
        return 'tiny'

    def __init__(self, output_dir):
        self._bench_conf = geopmpy.io.BenchConf(os.path.join(output_dir, 'tiny.conf'))
        self._bench_conf.append_region('dgemm', 0.2)
        self._bench_conf.set_loop_count(500)

    def get_num_node(self):
        # TODO: this seems bad
        # proposal: num_nodes always an input to __init__, app throws if it can't handle.
        # num ranks per node always an output from app conf.
        # if you want to play with different rank configs, use different app conf
        raise RuntimeError("<geopm> dgemm application scales to any number of nodes.")
        return None

    def get_rank_per_node(self):
        # TODO: use self._machine_file to determine?
        return 2

    def setup(self):
        self._bench_conf.write()
        return ""

    def get_exec_path(self):
        # TODO: may need to find local version if not installed
        return 'geopmbench'

    def get_exec_args(self):
        return self._bench_conf.get_path()
