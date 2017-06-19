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
import os
import sys
import unittest
import subprocess
import fnmatch
import time
import pandas
import collections

import geopm_test_launcher
import geopmpy.io

class TestIntegration(unittest.TestCase):
    def setUp(self):
        self.longMessage = True
        self._mode = 'dynamic'
        self._options = {'tree_decider' : 'static_policy',
                         'leaf_decider': 'power_governing',
                         'platform' : 'rapl',
                         'power_budget' : 150}
        self._tmp_files = []
        self._output = None

    def tearDown(self):
        if sys.exc_info() == (None, None, None) and os.getenv('GEOPM_KEEP_FILES') is None:
            if self._output is not None:
                self._output.remove_files()
            for ff in self._tmp_files:
                try:
                    os.remove(ff)
                except OSError:
                    pass

    def assertNear(self, a, b, epsilon=0.05):
        if abs(a - b) / a >= epsilon:
            self.fail('The fractional difference between {a} and {b} is greater than {epsilon}'.format(a=a, b=b, epsilon=epsilon))

    def test_report_and_trace_generation(self):
        name = 'test_report_and_trace_generation'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        for nn in node_names:
            report = self._output.get_report(nn)
            self.assertNotEqual(0, len(report))
            trace = self._output.get_trace(nn)
            self.assertNotEqual(0, len(trace))

    def test_report_and_trace_generation_pthread(self):
        name = 'test_report_and_trace_generation_pthread'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.set_pmpi_ctl('pthread')
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        for nn in node_names:
            report = self._output.get_report(nn)
            self.assertNotEqual(0, len(report))
            trace = self._output.get_trace(nn)
            self.assertNotEqual(0, len(trace))


    def test_report_and_trace_generation_application(self):
        name = 'test_report_and_trace_generation_pthread'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.set_pmpi_ctl('application')
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        for nn in node_names:
            report = self._output.get_report(nn)
            self.assertNotEqual(0, len(report))
            trace = self._output.get_trace(nn)
            self.assertNotEqual(0, len(trace))


    @unittest.skipUnless(geopm_test_launcher.resource_manager() == "SLURM", 'FIXME: Requires SLURM for alloc\'d and idle nodes.')
    def test_report_generation_all_nodes(self):
        name = 'test_report_generation_all_nodes'
        report_path = name + '.report'
        num_node=1
        num_rank=1
        delay = 1.0
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        time.sleep(5) # Wait a moment to finish cleaning-up from a previous test
        idle_nodes = launcher.get_idle_nodes()
        idle_nodes_copy = list(idle_nodes)
        alloc_nodes = launcher.get_alloc_nodes()
        launcher.write_log(name, 'Idle nodes : {nodes}'.format(nodes=idle_nodes))
        launcher.write_log(name, 'Alloc\'d  nodes : {nodes}'.format(nodes=alloc_nodes))
        node_names = []
        reports = {}
        for nn in idle_nodes_copy:
            launcher.set_node_list(nn.split()) # Hack to convert string to list
            try:
                launcher.run(name)
                node_names += nn.split()
                oo = geopmpy.io.AppOutput(report_path)
                reports[nn] = oo.get_report(nn)
            except subprocess.CalledProcessError as e:
                if e.returncode == 1 and nn not in launcher.get_idle_nodes():
                    launcher.write_log(name, '{node} has disappeared from the idle list!'.format(node=nn))
                    idle_nodes.remove(nn)
                else:
                    launcher.write_log(name, 'Return code = {code}'.format(code=e.returncode))
                    raise e

        self.assertEqual(len(node_names), len(idle_nodes))
        for nn in node_names:
            report = reports[nn]
            self.assertNotEqual(0, len(report))
            self.assertNear(delay, report['sleep'].get_runtime())
            self.assertGreater(report.get_runtime(), report['sleep'].get_runtime())
            self.assertEqual(1, report['sleep'].get_count())

    def test_runtime(self):
        name = 'test_runtime'
        report_path = name + '.report'
        num_node = 1
        num_rank = 5
        delay = 3.0
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', delay)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path)
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        for nn in node_names:
            rr = self._output.get_report(nn)
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())

    def test_runtime_nested(self):
        name = 'test_runtime_nested'
        report_path = name + '.report'
        num_node = 1
        num_rank = 1
        delay = 1.0
        loop_count = 2
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('nested-progress', delay)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path)
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        for nn in node_names:
            rr = self._output.get_report(nn)
            # The spin sections of this region sleep for 'delay' seconds twice per loop.
            self.assertNear(2 * loop_count * delay, rr['spin'].get_runtime())
            self.assertNear(rr['spin'].get_runtime(), rr['epoch'].get_runtime(), epsilon=0.01)
            self.assertGreater(rr.get_mpi_runtime(), 0)
            self.assertGreater(0.1, rr.get_mpi_runtime())
            self.assertEqual(loop_count, rr['spin'].get_count())

    def test_trace_runtimes(self):
        name = 'test_trace_runtimes'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)

        for nn in node_names:
            report = self._output.get_report(nn)
            trace = self._output.get_trace(nn)
            self.assertNear(trace.iloc[-1]['seconds'], report.get_runtime())

            # Calculate runtime totals for each region in each trace, compare to report
            tt = trace.set_index(['region_id'], append=True)
            tt = tt.groupby(level=['region_id'])
            for region_name, region_data in report.iteritems():
                if region_name != 'unmarked-region' and region_data.get_runtime() != 0:
                    trace_data = tt.get_group((region_data.get_id()))
                    trace_elapsed_time = trace_data.iloc[-1]['seconds'] - trace_data.iloc[0]['seconds']
                    self.assertNear(trace_elapsed_time, region_data.get_runtime())

    @unittest.skipUnless(os.getenv('GEOPM_RUN_LONG_TESTS') is not None,
    "Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    def test_region_runtimes(self):
        name = 'test_region_runtime'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        loop_count = 500
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 8.0)
        app_conf.set_loop_count(loop_count)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path, time_limit=900)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)

        # Calculate region times from traces
        region_times = collections.defaultdict(lambda: collections.defaultdict(dict))
        for nn in node_names:
            tt = self._output.get_trace(nn).set_index(['region_id'], append=True).groupby(level=['region_id'])

            for region_id, data in tt:
                # Build a df with only the first region entry and the exit.
                last_index = 0
                filtered_df = pandas.DataFrame()
                row_list = []
                progress_1s = data['progress-0'].loc[ data['progress-0'] == 1 ]
                for index, junk in progress_1s.iteritems():
                    row = data.ix[last_index:index].head(1)
                    row_list += [row[['seconds', 'progress-0']]]
                    row = data.ix[last_index:index].tail(1)
                    row_list += [row[['seconds', 'progress-0']]]
                    last_index = index[0] + 1 # Set the next starting index to be one past where we are
                filtered_df = pandas.concat(row_list)

                filtered_df = filtered_df.diff()
                # Since I'm not separating out the progress 0's from 1's, when I do the diff I only care about the
                # case where 1 - 0 = 1 for the progress column.
                filtered_df = filtered_df.loc[ filtered_df['progress-0'] == 1 ]

                if len(filtered_df) > 1:
                    launcher.write_log(name, 'Region elapsed time stats from {} - {} :\n{}'\
                    .format(nn, region_id, filtered_df['seconds'].describe()))
                    filtered_df['seconds'].describe()
                    region_times[nn][region_id] = filtered_df

            launcher.write_log(name, '{}'.format('-' * 80))

        # Loop through the reports to see if the region runtimes line up with what was calculated from the trace files above.
        write_regions = True
        for nn in node_names:
            rr = self._output.get_report(nn)
            for region_name, region in rr.iteritems():
                if region.get_id() != 0 and region.get_count() > 1:
                    if write_regions:
                        launcher.write_log(name, 'Region {} is {}.'.format(region.get_id(), region_name))
                    self.assertNear(region.get_runtime(),
                                    region_times[nn][region.get_id()]['seconds'].sum())
            write_regions = False

        # Test to ensure every region detected in the trace is captured in the report.
        for nn in node_names:
            rr = self._output.get_report(nn)
            report_ids = [rr[ii].get_id() for ii in rr]
            for region_id in region_times[nn].keys():
                self.assertTrue(region_id in report_ids, msg='Report from {} missing region_id {}'.format(nn, region_id))

    def test_progress(self):
        name = 'test_progress'
        report_path = name + '.report'
        num_node = 1
        num_rank = 4
        delay = 3.0
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep-progress', delay)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path)
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        for nn in node_names:
            rr = self._output.get_report(nn)
            self.assertNear(delay, rr['sleep'].get_runtime())
            self.assertGreater(rr.get_runtime(), rr['sleep'].get_runtime())
            self.assertEqual(1, rr['sleep'].get_count())

    def test_count(self):
        name = 'test_count'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 1
        num_rank = 4
        delay = 0.01
        loop_count = 100
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('spin', delay)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        for nn in node_names:
            rr = self._output.get_report(nn)
            self.assertNear(delay * loop_count, rr['spin'].get_runtime())
            self.assertEqual(loop_count, rr['spin'].get_count())
            self.assertEqual(loop_count, rr['epoch'].get_count())

        # TODO Trace file parsing + analysis

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
        num_node = 2
        loop_count = 100

        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        app_conf.set_loop_count(loop_count)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, time_limit=900)

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
                self._output = geopmpy.io.AppOutput(report_path)
                node_names = self._output.get_node_names()
                self.assertEqual(len(node_names), num_node)
                for nn in node_names:
                    rr = self._output.get_report(nn)
                    self.assertEqual(loop_count, rr['dgemm'].get_count())
                    self.assertEqual(loop_count, rr['all2all'].get_count())
                    self.assertGreater(rr['dgemm'].get_runtime(), 0.0)
                    self.assertGreater(rr['all2all'].get_runtime(), 0.0)
                num_node *= 2
                self._output.remove_files()

    @unittest.skipUnless(os.getenv('GEOPM_RUN_LONG_TESTS') is not None,
                         "Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    def test_power_consumption(self):
        name = 'test_power_consumption'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        loop_count = 500
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 8.0)
        app_conf.set_loop_count(loop_count)
        self._options['power_budget'] = 150
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path, time_limit=900)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.write_log(name, 'Power cap = {}W'.format(self._options['power_budget']))
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))
        all_power_data = {}
        # Total power consumed will be Socket(s) + DRAM
        for nn in node_names:
            rr = self._output.get_report(nn)
            tt = self._output.get_trace(nn)

            first_epoch_index = tt.loc[ tt['region_id'] == '9223372036854775808' ][:1].index[0]
            epoch_dropped_data = tt[first_epoch_index:] # Drop all startup data

            power_data = epoch_dropped_data.filter(regex='energy')
            power_data['seconds'] = epoch_dropped_data['seconds']
            power_data = power_data.diff().dropna()
            power_data.rename(columns={'seconds' : 'elapsed_time'}, inplace=True)
            power_data = power_data.loc[(power_data != 0).all(axis=1)] # Will drop any row that is all 0's

            pkg_energy_cols = [s for s in power_data.keys() if 'pkg_energy' in s]
            dram_energy_cols = [s for s in power_data.keys() if 'dram_energy' in s]
            power_data['socket_power'] = power_data[pkg_energy_cols].sum(axis=1) / power_data['elapsed_time']
            power_data['dram_power'] = power_data[dram_energy_cols].sum(axis=1) / power_data['elapsed_time']
            power_data['combined_power'] = power_data['socket_power'] + power_data['dram_power']

            pandas.set_option('display.width', 100)
            launcher.write_log(name, 'Power stats from {} :\n{}'.format(nn, power_data.describe()))

            all_power_data[nn] = power_data

        for node_name, power_data in all_power_data.iteritems():
            # Allow for overages of 2% at the 75th percentile.
            self.assertGreater(self._options['power_budget'] * 1.02, power_data['combined_power'].quantile(.75))

            # TODO Checks on the maximum power computed during the run?
            # TODO Checks to see how much power was left on the table?


    def test_progress_exit(self):
        """
        Check that when we always see progress exit before the next entry.
        Make sure that progress only decreases when a new region is entered.
        """
        name = 'test_progress_exit'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 1
        num_rank = 16
        loop_count = 100
        big_o = 0.01
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region('dgemm-progress', big_o)
        app_conf.append_region('spin-progress', 0.01)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))

        for nn in node_names:
            rr = self._output.get_report(nn)
            tt = self._output.get_trace(nn)

            tt = tt.set_index(['region_id'], append=True)
            tt = tt.groupby(level=['region_id'])

            for region_id, data in tt:
                tmp = data['progress-0'].diff()
                negative_progress =  tmp.loc[ (tmp > -1) & (tmp < 0) ]
                launcher.write_log(name, '{}'.format(negative_progress))
                self.assertEqual(0, len(negative_progress))

    @unittest.skipUnless(os.getenv('GEOPM_RUN_LONG_TESTS') is not None,
                         "Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    def test_sample_rate(self):
        """
        Check that sample rate is regular and fast.
        """
        name = 'test_sample_rate'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 1
        num_rank = 16
        loop_count = 10
        big_o = 10.0
        region = 'dgemm-progress'
        max_mean = 0.01 # 10 millisecond max sample period
        max_nstd = 0.1 # 10% normalized standard deviation (std / mean)
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.set_loop_count(loop_count)
        app_conf.append_region(region, big_o)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(num_node, len(node_names))

        for nn in node_names:
            rr = self._output.get_report(nn)
            tt = self._output.get_trace(nn)
            delta_t = tt['seconds'].diff()
            delta_t = delta_t.loc[ delta_t != 0]
            self.assertGreater(max_mean, delta_t.mean())
            # WARNING : The following line may mask issues in the sampling rate. To do a fine grained analysis, comment
            # out the next line and do NOT run on the BSP. This will require modifications to the launcher or manual testing.
            size_orig = len(delta_t)
            delta_t = delta_t[(delta_t - delta_t.mean()) < 3*delta_t.std()] # Only keep samples within 3 stds of the mean
            self.assertGreater(0.06, 1 - (float(len(delta_t)) / size_orig))
            self.assertGreater(max_nstd, delta_t.std() / delta_t.mean())

    def test_mpi_runtimes(self):
        name = 'test_mpi_runtimes'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        for nn in node_names:
            rr = self._output.get_report(nn)
            self.assertEqual(0, rr['unmarked-region'].get_count())
            self.assertNear(rr['all2all'].get_mpi_runtime(), rr['all2all'].get_runtime())
            self.assertEqual(rr['all2all'].get_mpi_runtime(), rr['epoch'].get_mpi_runtime())
            self.assertEqual(rr['all2all'].get_mpi_runtime(), rr.get_mpi_runtime())
            self.assertEqual(0, rr['unmarked-region'].get_mpi_runtime())
            self.assertEqual(0, rr['sleep'].get_mpi_runtime())
            self.assertEqual(0, rr['dgemm'].get_mpi_runtime())

    def test_ignore_runtime(self):
        name = 'test_ignore_runtimes'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('ignore', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path, trace_path + '*')
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        for nn in node_names:
            rr = self._output.get_report(nn)
            self.assertEqual(rr['ignore'].get_runtime(), rr.get_ignore_runtime())

    @unittest.skipUnless([True for line in
                          open(os.path.join(
                               os.path.dirname(
                               os.path.dirname(
                               os.path.realpath(__file__))),
                               'config.h'))
                          if line.startswith('#define GEOPM_ENABLE_OMPT')],
    "Configure with --enable-ompt to enable this test.")
    def test_unmarked_ompt(self):
        name = 'test_unmarked_ompt'
        report_path = name + '.report'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('stream-unmarked', 1.0)
        app_conf.append_region('dgemm-unmarked', 1.0)
        app_conf.append_region('all2all-unmarked', 1.0)
        ctl_conf = geopmpy.io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, ctl_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path)
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        stream_id = None
        stream_name = '[OMPT]geopm_test_integration:geopm::StreamModelRegion::run()'
        for nn in node_names:
            rr = self._output.get_report(nn)
            region_names = rr.keys()
            self.assertTrue(stream_name in rr)
            stream_region = rr[stream_name]
            self.assertEqual(1, stream_region.get_count())
            if stream_id:
                self.assertEqual(stream_id, stream_region.get_id())
            else:
                stream_id = stream_region.get_id()
            ompt_regions = [key for key in region_names if key.startswith('[OMPT]')]
            self.assertLessEqual(2, len(ompt_regions));
            self.assertTrue(('MPI_Alltoall' in rr));
            gemm_region = [key for key in region_names if key.lower().find('gemm') != -1]
            self.assertLessEqual(1, len(gemm_region))

    @unittest.skipUnless(False, 'Not implemented')
    def test_variable_end_time(self):
        """
        Check that two ranks on the same node can shutdown at
        different times without overflowing the table.
        """
        raise NotImplementedError

    @unittest.skipUnless(False, 'Not implemented')
    def test_power_excursion(self):
        """
        When a low power region is followed by a high power region, check
        that the power consumed at beginning of high power region does
        not excede the RAPL limit for too long.
        """
        raise NotImplementedError

    @unittest.skipUnless(False, 'Not implemented')
    def test_short_region_control(self):
        """
        Check that power limit is maintained when an application has many
        short running regions.
        """
        raise NotImplementedError

    @unittest.skipUnless(False, 'Not implemented')
    def test_mean_power_against_integration(self):
        """
        Check that the average of the per sample power is close to the
        total change in energy divided by the total change in time.
        """
        raise NotImplementedError


if __name__ == '__main__':
    unittest.main()
