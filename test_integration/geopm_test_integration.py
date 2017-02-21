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
import pandas
import collections

import geopm_launcher
import geopm_io

class TestIntegration(unittest.TestCase):
    def setUp(self):
        self._mode = 'dynamic'
        self._options = {'tree_decider' : 'static_policy',
                         'leaf_decider': 'power_governing',
                         'platform' : 'rapl',
                         'power_budget' : 150}
        self._tmp_files = []

    def tearDown(self):
        if sys.exc_info() == (None, None, None): # Will not be none if handling exception (i.e. failing test)
            if os.getenv('GEOPM_KEEP_FILES') is None:
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
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)
        traces = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, trace_path + '*')]
        self._tmp_files.extend(traces)
        self.assertTrue(len(traces) == num_node)
        for ff in traces:
            self.assertTrue(os.path.isfile(ff))
            self.assertTrue(os.stat(ff).st_size != 0)
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

    def test_trace_runtimes(self):
        name = 'test_trace_generation'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('sleep', 1.0)
        app_conf.append_region('dgemm', 1.0)
        app_conf.append_region('all2all', 1.0)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, trace_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')] # File names list
        self._tmp_files.extend(reports)
        reports = [geopm_io.Report(rr) for rr in reports] # Report objects list
        reports = {rr.get_node_name(): rr for rr in reports} # Report objects dict

        traces = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, trace_path + '*')]
        self._tmp_files.extend(traces)
        # Create a dict of <NODE_NAME> : <TRACE_DATAFRAME>
        traces = {tt.split('.trace-')[-1] : geopm_io.Trace(tt).get_df() for tt in traces}

        # Calculate runtime totals for each trace, compare to report
        for node_name, node_trace in traces.iteritems():
            self.assertNear(node_trace.iloc[-1]['seconds'], reports[node_name].get_runtime())

        for node_name, report in reports.iteritems():
            # Calculate runtime totals for each region in each trace, compare to report
            t = traces[node_name].set_index(['region_id'], append=True)
            t = t.groupby(level=['region_id'])
            for region_name, report_data in report.iteritems():
                if report_data.get_runtime() == 0:
                    continue
                trace_data = t.get_group((report_data.get_id()))
                trace_elapsed_time = trace_data.iloc[-1]['seconds'] - trace_data.iloc[0]['seconds']
                self.assertNear(trace_elapsed_time, report_data.get_runtime())

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
        trace_path = name + '.trace'
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
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, trace_path, time_limit=None)
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

    @unittest.skipUnless(os.getenv('GEOPM_RUN_LONG_TESTS') is not None,
                         "Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    def test_power_consumption(self):
        name = 'test_power_consumption'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        loop_count = 500
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 8.0)
        app_conf.set_loop_count(loop_count)
        self._options['power_budget'] = 150
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, trace_path, time_limit=15)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.write_log(name, 'Power cap = {}W'.format(self._options['power_budget']))
        launcher.run(name)

        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)

        traces = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, trace_path + '*')]
        self._tmp_files.extend(traces)
        # Create a dict of <NODE_NAME> : <TRACE_DATAFRAME>
        traces = {tt.split('.trace-')[-1] : geopm_io.Trace(tt).get_df() for tt in traces}

        # Total power consumed will be Socket(s) + DRAM
        all_power_data = {}
        for node_name, trace_data in traces.iteritems():
            first_epoch_index = trace_data.loc[ trace_data['region_id'] == '9223372036854775808' ][:1].index[0]
            epoch_dropped_data = trace_data[first_epoch_index:] # Drop all startup data

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
            launcher.write_log(name, 'Power stats from {} :\n{}'.format(node_name, power_data.describe()))

            all_power_data[node_name] = power_data

        for node_name, power_data in all_power_data.iteritems():
            # Allow for overages of 1% at the 75th percentile.
            self.assertGreater(self._options['power_budget'] * 1.01, power_data['combined_power'].quantile(.75))

            # TODO Checks on the maximum power computed during the run?
            # TODO Checks to see how much power was left on the table?

    @unittest.skipUnless(os.getenv('GEOPM_RUN_LONG_TESTS') is not None,
                         "Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    def test_region_runtime(self):
        name = 'test_region_runtime'
        report_path = name + '.report'
        trace_path = name + '.trace'
        num_node = 4
        num_rank = 16
        loop_count = 500
        app_conf = geopm_io.AppConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('dgemm', 8.0)
        app_conf.set_loop_count(loop_count)
        ctl_conf = geopm_io.CtlConf(name + '_ctl.config', self._mode, self._options)
        self._tmp_files.append(ctl_conf.get_path())
        launcher = geopm_launcher.factory(app_conf, ctl_conf, report_path, trace_path, time_limit=15)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        reports = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, report_path + '*')]
        self._tmp_files.extend(reports)
        reports = [geopm_io.Report(rr) for rr in reports] # Report objects list
        reports = {rr.get_node_name(): rr for rr in reports} # Report objects dict

        traces = [ff for ff in os.listdir('.') if fnmatch.fnmatch(ff, trace_path + '*')]
        self._tmp_files.extend(traces)
        # Create a dict of <NODE_NAME> : <TRACE_DATAFRAME>
        traces = {tt.split('.trace-')[-1] : geopm_io.Trace(tt).get_df() for tt in traces}

        # Calculate region times from traces
        region_times = collections.defaultdict(lambda: collections.defaultdict(dict))
        for node_name, trace_data in traces.iteritems():
            t = trace_data.set_index(['region_id'], append=True)
            t = t.groupby(level=['region_id'])
            for region_id, data in t:
                if region_id == '0':
                    continue

                region_start = -1
                filtered_df = pandas.DataFrame()
                for index, row in data.iterrows():
                    if region_start == -1 and row['progress-0'] == 0:
                        region_start = index
                        filtered_df = filtered_df.append(row[['seconds', 'progress-0']])
                    elif row['progress-0'] == 1:
                        filtered_df = filtered_df.append(row[['seconds', 'progress-0']])
                        region_start = -1

                filtered_df = filtered_df.diff()
                # Since I'm not separating out the progress 0's from 1's, when I do the diff I only care about the
                # case where 1 - 0 = 1 for the progress column.
                filtered_df = filtered_df.loc[ filtered_df['progress-0'] == 1 ]

                if len(filtered_df) > 1:
                    launcher.write_log(name, 'Region elapsed time stats from {} - {} :\n{}'\
                                       .format(node_name, region_id, filtered_df['seconds'].describe()))
                    filtered_df['seconds'].describe()
                    region_times[node_name][region_id] = filtered_df

            launcher.write_log(name, '\n{}'.format('-' * 80))

        # Loop through the reports to see if the region runtimes line up with what was calculated from the trace files above.
        write_regions = True
        for node_name, report in reports.iteritems():
            for region_name, region in report.iteritems():
                if region.get_id() == 0 or region.get_count() <= 1:
                    continue
                if write_regions:
                    launcher.write_log(name, 'Region {} is {}.'.format(region.get_id(), region_name))
                self.assertNear(region.get_runtime(),
                                region_times[node_name][region.get_id()]['seconds'].sum())
            write_regions = False


if __name__ == '__main__':
    unittest.main()
