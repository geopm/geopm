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

class Report(dict):
    def __init__(self, report_path):
        super(Report, self).__init__()
        self._path = report_path
        with open(report_path, 'r') as fid:
            (region_name, runtime, energy, frequency, count) = None, None, None, None, None

            for line in fid:
                match = re.search(r'^##### geopm (\S+) #####$', line)
                if match is not None:
                    self._version = match.group(1)
                    continue

                match = re.search(r'^Profile: \.\/(\S+)$', line)
                if match is not None:
                    self._name = match.group(1)
                    continue

                match = re.search(r'^Region (\S+):', line)
                if match is not None:
                    region_name = match.group(1)
                    continue

                match = re.search(r'^\s+runtime.+: (\d*\.\d+|\d+)', line)
                if match is not None:
                    runtime = float(match.group(1))
                    continue

                match = re.search(r'^\s+energy.+: (\d*\.\d+|\d+)', line)
                if match is not None:
                    energy = float(match.group(1))
                    continue

                match = re.search(r'^\s+frequency.+: (\d*\.\d+|\d+)', line)
                if match is not None:
                    frequency = float(match.group(1))
                    continue

                match = re.search(r'^\s+count: (\d*\.\d+|\d+)', line)
                if match is not None:
                    count = float(match.group(1))

                if None not in (region_name, runtime, energy, frequency, count):
                    self[region_name] = Region(region_name, runtime, energy, frequency, count)
                    (region_name, runtime, energy, frequency, count) = None, None, None, None, None

    def get_name(self):
        return self._name

    def get_version(self):
        return self._version

    def get_region(self, name):
        try:
            return self[name]
        except KeyError:
            return ZeroRegion(name)

    def get_path(self):
        return self._path

class Region(object):
    def __init__(self, name, runtime, energy, frequency, count):
        self._name = name
        self._runtime = runtime
        self._energy = energy
        self._frequency = frequency
        self._count = count

    def __repr__(self):
        return self._name + "\n  Runtime : %f" % self._runtime + "\n  Energy : %f" % self._energy \
            + "\n  Frequency : %f" % self._frequency + "\n  Count : %f" % self._count

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

class ZeroRegion(Region):
    def __init__(self, name):
        self._name = name
        self._runtime = 0.0
        self._energy = 0.0
        self._frequency = 0.0
        self._count = 0.0

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
                     trace_path=None, host_file=None):
    hostname = socket.gethostname()
    if hostname.find('mr-fusion') == 0:
        return SrunLauncher(app_conf, ctl_conf, report_path,
                            trace_path, host_file)
    else:
        raise LookupError('Unrecognized hostname: ' + hostname)

class Launcher(object):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None):
        self._num_rank = 16
        self._num_node = 4
        self._app_conf = app_conf
        self._ctl_conf = ctl_conf
        self._report_path = report_path
        self._trace_path = trace_path
        self._host_file = host_file
        self._pmpi_ctl = 'process'
        # Figure out the number of CPUs per rank leaving one for the
        # OS and one (potentially) for the controller.
        self._num_thread = (multiprocessing.cpu_count() - 2) / self._num_rank


    def set_num_node(self, num_node):
        self._num_node = num_node

    def set_num_rank(self, num_rank):
        self._num_rank = num_rank
        self._num_thread = multiprocessing.cpu_count() / self._num_rank

    def set_pmpi_ctl(self, pmpi_ctl):
        self._pmpi_ctl = pmpi_ctl

    def run(self, test_name):
        env = dict(os.environ)
        env.update(self._environ())
        self._app_conf.write()
        self._ctl_conf.write()
        with open(test_name + '.log', 'w') as outfile:
            subprocess.check_call(self._exec_str(), shell=True, env=env, stdout=outfile, stderr=outfile)

    def get_report(self):
        return Report(self._report_path)

    def get_trace(self):
        return Trace(self._trace_path)

    def _environ(self):
        result = {'LD_DYNAMIC_WEAK': 'true',
                  'OMP_NUM_THREADS' : str(self._num_thread),
                  'GEOPM_PMPI_CTL' : self._pmpi_ctl,
                  'GEOPM_REPORT' : self._report_path,
                  'GEOPM_POLICY' : self._ctl_conf.get_path()}
        if (self._trace_path):
            result['GEOPM_TRACE'] = self._trace_path
        return result

    def _exec_str(self):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        exec_path = os.path.join(script_dir, 'geopm_test_integration')
        return ' '.join((self._libtool_option(),
                         self._mpiexec_option(),
                         self._num_node_option(),
                         self._num_rank_option(),
                         self._affinity_option(),
                         self._host_option(),
                         self._membind_option(),
                         exec_path,
                         self._app_conf.get_path()))


    def _num_rank_option(self):
        return '-n {num_rank}'.format(num_rank=self._num_rank)

    def _affinity_option(self):
        return ''

    def _membind_option(self):
        return ''

    def _libtool_option(self):
        return 'libtool --mode=execute'

    def _host_option(self):
        if self._host_file:
            throw NotImplementedError
        return ''


class SrunLauncher(Launcher):
    def __init__(self, app_conf, ctl_conf, report_path,
                 trace_path=None, host_file=None):
        super(SrunLauncher, self).__init__(app_conf, ctl_conf, report_path,
                                           trace_path=trace_path, host_file=host_file)

    def _mpiexec_option(self):
        return 'srun'

    def _num_node_option(self):
        return '-N {num_node}'.format(num_node=self._num_node)

    def _affinity_option(self):
        proc_mask = self._num_thread * '1' + '00'
        result_base = '--cpu_bind=v,mask_cpu:'
        mask_list = []
        if (self._pmpi_ctl == 'process'):
            mask_list.append('0x2,')
        for ii in range(self._num_rank):
            mask_list.append('0x{:x}'.format(int(proc_mask, 2)))
            proc_mask = proc_mask + self._num_thread * '0'
        return result_base + ','.join(mask_list)

    def _host_option(self):
        result = ''
        if self._host_file:
            result = '-w {host_file}'.format(self._host_file)
        return result

class TestReport(unittest.TestCase):
    def setUp(self):
        self._mode = 'dynamic'
        self._options = {'tree_decider' : 'static_policy',
                         'leaf_decider': 'power_governing',
                         'platform' : 'rapl',
                         'power_budget' : 150}
        self._epsilon = 0.05
        self._tmp_files = []

    def tearDown(self):
        for ff in self._tmp_files:
            os.remove(ff)

    def assertNear(self, a, b):
        if abs(a - b) / a >= self._epsilon:
            self.fail('The fractional difference between {a} and {b} is greater than {epsilon}'.format(a=a, b=b, epsilon=self._epsilon))

    def test_report_generation(self):
        name = 'test_report_generation'
        report_path = name + '.report'
        num_node = 4
        num_rank = 20
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
            self.assertGreater(rr['outer-sync'].get_runtime, rr['sleep'].get_runtime())

    def test_progress(self):
        name = 'test_progress'
        report_path = name + '.report'
        num_node = 1
        num_rank = 5
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
            self.assertGreater(rr['outer-sync'].get_runtime, rr['sleep'].get_runtime())

if __name__ == '__main__':
    unittest.main()
