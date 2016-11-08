#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, Intel Corporation
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

import unittest
import subprocess
import os
import json
import re
import fnmatch
import multiprocessing
import socket
import sys
import time


class Report(dict):
    def __init__(self, report_path):
        super(Report, self).__init__()
        self._path = report_path
        self._version = None
        self._name = None
        self._total_runtime = None
        self._total_energy = None
        self._total_mpi_runtime = None
        found_totals = False
        (region_name, runtime, energy, frequency, count) = None, None, None, None, None
        float_regex = r'([-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?)'

        with open(self._path, 'r') as fid:
            for line in fid:
                if self._version is None:
                    match = re.search(r'^##### geopm (\S+) #####$', line)
                    if match is not None:
                        self._version = match.group(1)
                elif self._name is None:
                    match = re.search(r'^Profile: (\S+)$', line)
                    if match is not None:
                        self._name = match.group(1)
                elif region_name is None:
                    match = re.search(r'^Region (\S+):', line)
                    if match is not None:
                        region_name = match.group(1)
                elif runtime is None:
                    match = re.search(r'^\s+runtime.+: ' + float_regex, line)
                    if match is not None:
                        runtime = float(match.group(1))
                elif energy is None:
                    match = re.search(r'^\s+energy.+: ' + float_regex, line)
                    if match is not None:
                        energy = float(match.group(1))
                elif frequency is None:
                    match = re.search(r'^\s+frequency.+: ' + float_regex, line)
                    if match is not None:
                        frequency = float(match.group(1))
                elif count is None:
                    match = re.search(r'^\s+count: ' + float_regex, line)
                    if match is not None:
                        count = float(match.group(1))
                        self[region_name] = Region(region_name, runtime, energy, frequency, count)
                        (region_name, runtime, energy, frequency, count) = None, None, None, None, None
                if not found_totals:
                    match = re.search(r'^Application Totals:$', line)
                    if match is not None:
                        found_totals = True
                elif self._total_runtime is None:
                    match = re.search(r'\s+runtime.+: ' + float_regex, line)
                    if match is not None:
                        self._total_runtime = float(match.group(1))
                elif self._total_energy is None:
                    match = re.search(r'\s+energy.+: ' + float_regex, line)
                    if match is not None:
                        self._total_energy = float(match.group(1))
                elif self._total_mpi_runtime is None:
                    match = re.search(r'\s+mpi-runtime.+: ' + float_regex, line)
                    if match is not None:
                        self._total_mpi_runtime = float(match.group(1))

        if (region_name is not None or not found_totals or
            None in (self._name, self._version, self._total_runtime, self._total_energy, self._total_mpi_runtime)):
            raise SyntaxError('Unable to parse file: ' + self._path)

    def get_name(self):
        return self._name

    def get_version(self):
        return self._version

    def get_path(self):
        return self._path

    def get_runtime(self):
        return self._total_runtime

    def get_mpi_runtime(self):
        return self._total_mpi_runtime

    def get_energy(self):
        return self._total_energy


class Region(object):
    def __init__(self, name, runtime, energy, frequency, count):
        self._name = name
        self._runtime = runtime
        self._energy = energy
        self._frequency = frequency
        self._count = count

    def __repr__(self):
        template = """\
{name}
  Runtime   : {runtime}
  Energy    : {energy}
  Frequency : {frequency}
  Count     : {count}
"""
        return template.format(name=self._name,
                               runtime=self._runtime,
                               energy=self._energy,
                               frequency=self._frequency,
                               count=self._count)

    def __str__(self):
        return self.__repr__()

    def get_name(self):
        return self._name

    def get_runtime(self):
        return self._runtime

    def get_energy(self):
        return self._energy

    def get_frequency(self):
        return self._frequency

    def get_count(self):
        return self._count

class Trace(object):
    def __init__(self, trace_path):
        raise NotImplementedError

class AppConf(object):
    def __init__(self, path):
        self._path = path
        self._loop_count = 1;
        self._region = []
        self._big_o = []
        self._hostname = []
        self._imbalance = []

    def set_loop_count(self, loop_count):
        self._loop_count = loop_count

    def append_region(self, name, big_o):
        self._region.append(name)
        self._big_o.append(big_o)

    def append_imbalance(self, hostname, imbalance):
        self._hostname.append(hostname)
        self._imbalance.append(imbalance)

    def get_path(self):
        return self._path

    def write(self):
        obj = {'loop-count' : self._loop_count,
               'region' : self._region,
               'big-o' : self._big_o}

        if (self._imbalance and self._hostname):
            obj['imbalance'] = self._imbalance
            obj['hostname'] = self._hostname

        with open(self._path, 'w') as fid:
            json.dump(obj, fid)


class CtlConf(object):
    def __init__(self, path, mode, options):
        self._path = path
        self._mode = mode
        self._options = options

    def set_tree_decider(self, decider):
        self._options['tree_decider'] = decider

    def set_leaf_decider(self, decider):
        self._options['leaf_decider'] = decider

    def set_platform(self, platform):
        self._options['platform'] = platform

    def set_power_budget(self, budget):
        self._options['power_budget'] = budget

    def get_path(self):
        return self._path

    def write(self):
        obj = {'mode' : self._mode,
               'options' : self._options}
        with open(self._path, 'w') as fid:
            json.dump(obj, fid)


def launcher_factory(app_conf, ctl_conf, report_path,
                     trace_path=None, host_file=None, time_limit=1):
    hostname = socket.gethostname()
    if hostname.find('mr-fusion') == 0:
        return SrunLauncher(app_conf, ctl_conf, report_path,
                            trace_path, host_file, time_limit)
    else:
        raise LookupError('Unrecognized hostname: ' + hostname)


class Launcher(object):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None, time_limit=None, region_barrier=False):
        self._app_conf = app_conf
        self._ctl_conf = ctl_conf
        self._report_path = report_path
        self._trace_path = trace_path
        self._host_file = host_file
        self._time_limit = time_limit
        self._region_barrier = region_barrier
        self._node_list = None
        self._pmpi_ctl = 'process'
        self.set_num_rank(16)
        self.set_num_node(4)

    def __repr__(self):
        output = ''
        for k,v in self._environ().iteritems():
            output += '{k}={v} '.format(k=k, v=v)
        output += self._exec_str()
        return output

    def __str__(self):
        return self.__repr__()

    def set_node_list(self, node_list):
        self._node_list = node_list

    def set_num_node(self, num_node):
        self._num_node = num_node
        self._set_num_thread()

    def set_num_rank(self, num_rank):
        self._num_rank = num_rank
        self._set_num_thread()

    def set_pmpi_ctl(self, pmpi_ctl):
        self._pmpi_ctl = pmpi_ctl

    def run(self, test_name):
        env = dict(os.environ)
        env.update(self._environ())
        self._app_conf.write()
        self._ctl_conf.write()
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(str(self) + '\n\n')
            outfile.flush()
            subprocess.check_call(self._exec_str(), shell=True, env=env, stdout=outfile, stderr=outfile)

    def get_report(self):
        return Report(self._report_path)

    def get_trace(self):
        return Trace(self._trace_path)

    def get_idle_nodes(self):
        return ''

    def get_alloc_nodes(self):
        return ''

    def write_log(self, test_name, message):
        with open(test_name + '.log', 'a') as outfile:
            outfile.write(message + '\n\n')

    def _set_num_thread(self):
        # Figure out the number of CPUs per rank leaving one for the
        # OS and one (potentially, may/may not be use depending on pmpi_ctl)
        # for the controller.
        try:
            self._num_thread = (multiprocessing.cpu_count() - 2) / (self._num_rank / self._num_node)
        except AttributeError:
            pass

    def _environ(self):
        result = {'LD_DYNAMIC_WEAK': 'true',
                  'OMP_NUM_THREADS' : str(self._num_thread),
                  'GEOPM_PMPI_CTL' : self._pmpi_ctl,
                  'GEOPM_REPORT' : self._report_path,
                  'GEOPM_POLICY' : self._ctl_conf.get_path()}
        if self._trace_path:
            result['GEOPM_TRACE'] = self._trace_path
        if self._region_barrier:
            result['GEOPM_REGION_BARRIER'] = 'true'
        return result

    def _exec_str(self):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        # Using libtool causes sporadic issues with the Intel toolchain.
        exec_path = os.path.join(script_dir, '.libs', 'geopm_test_integration --verbose')
        return ' '.join((self._mpiexec_option(),
                         self._num_node_option(),
                         self._num_rank_option(),
                         self._affinity_option(),
                         self._host_option(),
                         self._membind_option(),
                         exec_path,
                         self._app_conf.get_path()))

    def _num_rank_option(self):
        num_rank = self._num_rank
        if self._pmpi_ctl == 'process':
            num_rank += self._num_node
        return '-n {num_rank}'.format(num_rank=num_rank)

    def _affinity_option(self):
        return ''

    def _membind_option(self):
        return ''

    def _host_option(self):
        if self._host_file:
            raise NotImplementedError
        return ''


class SrunLauncher(Launcher):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None, time_limit=1):
        super(SrunLauncher, self).__init__(app_conf, ctl_conf, report_path,
                                           trace_path=trace_path, host_file=host_file, time_limit=time_limit)
        self._queuing_timeout = 30
        self._job_name = 'int_test'

    def _mpiexec_option(self):
        mpiexec = 'srun -I{timeout} -J {name}'.format(timeout=self._queuing_timeout, name=self._job_name)
        if self._time_limit is not None:
            mpiexec += ' -t {time_limit}'.format(time_limit=self._time_limit)
        if self._node_list is not None:
            mpiexec += ' -w ' + ','.join(self._node_list)
        return mpiexec

    def _num_node_option(self):
        return '-N {num_node}'.format(num_node=self._num_node)

    def _affinity_option(self):
        proc_mask = self._num_thread * '1' + '00'
        result_base = '--cpu_bind=v,mask_cpu:'
        mask_list = []
        if (self._pmpi_ctl == 'process'):
            mask_list.append('0x2')
        for ii in range(self._num_rank / self._num_node):
            mask_list.append('0x{:x}'.format(int(proc_mask, 2)))
            proc_mask = proc_mask + self._num_thread * '0'
        return result_base + ','.join(mask_list)

    def _host_option(self):
        result = ''
        if self._host_file:
            result = '-w {host_file}'.format(self._host_file)
        return result

    def get_idle_nodes(self):
        return subprocess.check_output('sinfo -t idle -hNo %N', shell=True).splitlines()

    def get_alloc_nodes(self):
        return subprocess.check_output('sinfo -t alloc -hNo %N', shell=True).splitlines()


class TestReport(unittest.TestCase):
    def setUp(self):
        self._mode = 'dynamic'
        self._options = {'tree_decider' : 'static_policy',
                         'leaf_decider': 'power_governing',
                         'platform' : 'rapl',
                         'power_budget' : 150}
        self._tmp_files = []

    def tearDown(self):
        if sys.exc_info() == (None, None, None): # Will not be none if handling exception (i.e. failing test)
            for ff in self._tmp_files:
                try:
                    os.remove(ff)
                except OSError:
                    pass

    def assertNear(self, a, b, epsilon=0.05):
        if abs(a - b) / a >= epsilon:
            self.fail('The fractional difference between {a} and {b} is greater than {epsilon}'.format(a=a, b=b, epsilon=epsilon))

    def test_report_generation(self):
        name = 'test_report_generation'
        report_path = name + '.report'
        num_node = 4
        num_rank = 16
        app_conf = AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        ctl_conf = CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = launcher_factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        self.assertTrue(len(reports) == num_node)
        for ff in reports:
            self.assertTrue(os.path.isfile(ff))
            self.assertTrue(os.stat(ff).st_size != 0)

    def test_report_generation_all_nodes(self):
        name = 'test_report_generation_all_nodes'
        report_path = name + '.report'
        num_node=1
        num_rank=1
        delay = 1.0
        app_conf = AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        ctl_conf = CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = launcher_factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        time.sleep(5) # Wait a moment to finish cleaning-up from a previous test
        idle_nodes = launcher.get_idle_nodes()
        idle_nodes_copy = list(idle_nodes)
        alloc_nodes = launcher.get_alloc_nodes()
        launcher.write_log(name, 'Idle nodes : {nodes}'.format(nodes=idle_nodes))
        launcher.write_log(name, 'Alloc\'d  nodes : {nodes}'.format(nodes=alloc_nodes))
        for n in idle_nodes_copy:
            launcher.set_node_list(n.split()) # Hack to convert string to list
            try:
                launcher.run(name)
            except subprocess.CalledProcessError as e:
                if e.returncode == 1 and n not in launcher.get_idle_nodes():
                    launcher.write_log(name, '{node} has disappeared from the idle list!'.format(node=n))
                    idle_nodes.remove(n)
                else:
                    launcher.write_log(name, 'Return code = {code}'.format(code=e.returncode))
                    raise e
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [Report(rr) for rr in reports]
        self.assertTrue(len(reports) == len(idle_nodes))
        for rr in reports:
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())

    def test_runtime(self):
        name = 'test_runtime'
        report_path = name + '.report'
        num_node = 1
        num_rank = 5
        delay = 3.0
        app_conf = AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        ctl_conf = CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = launcher_factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [Report(rr) for rr in reports]
        self.assertTrue(len(reports) == num_node)
        for rr in reports:
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())

    def test_runtime_nested(self):
        name = 'test_runtime_nested'
        report_path = name + '.report'
        num_node = 1
        num_rank = 1
        delay = 1.0
        loop_count = 2
        app_conf = AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('nested-progress', delay)
        ctl_conf = CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = launcher_factory(app_conf, ctl_conf, report_path, time_limit=None)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [Report(rr) for rr in reports]
        self.assertTrue(len(reports) == num_node)
        for rr in reports:
            # The spin sections of this region sleep for 'delay' seconds twice per loop.
            self.assertNear(delay*loop_count*2, rr['spin'].get_runtime())
            self.assertNear(rr['spin'].get_runtime(), rr['epoch'].get_runtime(), epsilon=0.01)
            self.assertGreater(rr.get_mpi_runtime(), 0)
            self.assertGreater(0.1, rr.get_mpi_runtime())

    def test_progress(self):
        name = 'test_progress'
        report_path = name + '.report'
        num_node = 1
        num_rank = 4
        delay = 3.0
        app_conf = AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep-progress', delay)
        ctl_conf = CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = launcher_factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [Report(rr) for rr in reports]
        self.assertTrue(len(reports) == num_node)
        for rr in reports:
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())

if __name__ == '__main__':
    unittest.main()
