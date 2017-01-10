#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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
import fnmatch
import sys
import time

import geopm_launcher
import geopm_io


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
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        self.assertTrue(len(reports) == num_node)
        for ff in reports:
            self.assertTrue(os.path.isfile(ff))
            self.assertTrue(os.stat(ff).st_size != 0)

    @unittest.skipUnless(geopm_launcher.get_resource_manager() == "SLURM", 'FIXME: Requires SLURM for alloc\'d and idle nodes.')
    def test_report_generation_all_nodes(self):
        name = 'test_report_generation_all_nodes'
        report_path = name + '.report'
        num_node=1
        num_rank=1
        delay = 1.0
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path)
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
        reports = [geopm_io.Report(rr) for rr in reports]
        self.assertTrue(len(reports) == len(idle_nodes))
        for rr in reports:
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())
            self.assertEqual(1, rr['sleep'].get_count())

    def test_runtime(self):
        name = 'test_runtime'
        report_path = name + '.report'
        num_node = 1
        num_rank = 5
        delay = 3.0
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [geopm_io.Report(rr) for rr in reports]
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
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('nested-progress', delay)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, time_limit=None)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [geopm_io.Report(rr) for rr in reports]
        self.assertTrue(len(reports) == num_node)
        for rr in reports:
            # The spin sections of this region sleep for 'delay' seconds twice per loop.
            self.assertNear(2 * loop_count * delay, rr['spin'].get_runtime())
            self.assertNear(rr['spin'].get_runtime(), rr['epoch'].get_runtime(), epsilon=0.01)
            self.assertGreater(rr.get_mpi_runtime(), 0)
            self.assertGreater(0.1, rr.get_mpi_runtime())
            self.assertEqual(loop_count, rr['spin'].get_count())

    def test_progress(self):
        name = 'test_progress'
        report_path = name + '.report'
        num_node = 1
        num_rank = 4
        delay = 3.0
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep-progress', delay)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [geopm_io.Report(rr) for rr in reports]
        self.assertTrue(len(reports) == num_node)
        for rr in reports:
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())
            self.assertEqual(1, rr['sleep'].get_count())

    def test_count(self):
        name = 'test_count'
        report_path = name + '.report'
        num_node = 1
        num_rank = 4
        delay = 0.001
        loop_count = 500
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('spin', delay)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, time_limit=None)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [geopm_io.Report(rr) for rr in reports]
        self.assertTrue(len(reports) == num_node)
        for rr in reports:
            self.assertNear(delay * loop_count, rr['spin'].get_runtime())
            self.assertEqual(loop_count, rr['spin'].get_count())
            self.assertEqual(loop_count, rr['epoch'].get_count())

    @unittest.skipUnless(os.getenv('GEOPM_RUN_LONG_TESTS') is not None,
                         "Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    def test_scaling(self):
        """
        This test will start at ${num_node} nodes and ranks.  It will then calls check_run() to
        ensure that commands can be executed successfully on all of the allocated compute nodes.
        Afterwards it will run the specified app config on each node and verify the reports.  When
        complete it will double num_node and run the steps again.

        WARNING: This test can take a long time to run depending on the number of starting nodes and
        the size of the allocation.
        """
        name = 'test_scaling'
        report_path = name + '.report'
        num_node = 1
        loop_count = 100

        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        app_conf.set_loop_count(loop_count)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, time_limit=None)

        check_successful = True
        while check_successful:
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_node)
            try:
                launcher.check_run(name)
            except subprocess.CalledProcessError as e:
                # If we exceed the available nodes in the allocation ALPS/SLURM give a rc of 1
                # All other rc's are real errors
                if e.returncode != 1:
                    raise e
                check_successful = False
            if check_successful:
                launcher.write_log(name, 'About to run on {} nodes.'.format(num_node))
                launcher.run(name)
                report_paths = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
                self._tmp_files.extend(report_paths)
                reports = [geopm_io.Report(rr) for rr in report_paths]
                self.assertTrue(len(reports) == num_node)
                for rr in reports:
                    self.assertEqual(loop_count, rr['dgemm'].get_count())
                    self.assertEqual(loop_count, rr['all2all'].get_count())
                    self.assertGreater(rr['dgemm'].get_runtime(), 0.0)
                    self.assertGreater(rr['all2all'].get_runtime(), 0.0)
                for ff in report_paths:
                    try:
                        os.remove(ff)
                    except OSError:
                        pass
                num_node *= 2

if __name__ == '__main__':
    unittest.main()
