#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""EPOCH_INFERENCE

This test exercises the --geopm-region-filter option that allows
epochs to be inferred from a specific region in the application.

The test application contains the following sequence of regions in its
main loop:

loop:
    spin()
    spin()
    all2all()

Without a region filter to infer epochs, no epochs will be counted in
the trace.  If the proxy_epoch filter is used with the spin region as
the proxy, the number of epochs counted will be 2x the loop iteration
count.  If all2all is used as the proxy, the epoch count will
match the loop iteration count.

"""

import sys
import unittest
import os
import glob

import geopmpy.io
import geopmpy.agent
import geopmdpy.error

from integration.test import util
if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    from integration.test import geopm_test_launcher


class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the epoch_inference benchmark.

    """
    def write(self):
        """Called by the test launcher prior to executing the test application
        to write any files required by the application.

        """
        pass

    def get_exec_path(self):
        """Path to benchmark filled in by template automatically.

        """
        script_dir = os.path.dirname(os.path.realpath(__file__))
        return os.path.join(script_dir, '.libs', 'test_epoch_inference')

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


class TestIntegration_epoch_inference(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'epoch_inference'
        cls._skip_launch = not util.do_launch()

        cls._files = []
        cls._trace_path_prefix = 'test_{}_trace'.format(cls._test_name)
        cls._agent_conf_path = 'test_' + cls._test_name + '-agent-config.json'
        cls._files.append(cls._agent_conf_path)

        cls._config_names = ['no_epoch', 'spin_epoch', 'barrier_epoch', 'spin_stride_epoch']

        if not cls._skip_launch:
            num_node = 1
            num_rank = 1
            time_limit = 6000
            app_conf = AppConf()
            agent_conf = geopmpy.agent.AgentConf(cls._agent_conf_path,
                                                 'monitor',
                                                 {})
            for run_config in cls._config_names:
                report_path = 'test_{}_{}.report'.format(cls._test_name, run_config)
                trace_path = '{}_{}'.format(cls._trace_path_prefix, run_config)
                profile_trace_path = '{}_profile_{}'.format(cls._trace_path_prefix, run_config)
                cls._files.append(report_path)

                # Create the test launcher with the above configuration
                launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                            agent_conf,
                                                            report_path,
                                                            trace_path,
                                                            time_limit=time_limit,
                                                            trace_profile_path=profile_trace_path)
                launcher.set_num_node(num_node)
                launcher.set_num_rank(num_rank)
                # Run the test application
                geopm_args = []
                if run_config == 'spin_epoch':
                    geopm_args = ['--geopm-record-filter', 'proxy_epoch,spin']
                elif run_config == 'barrier_epoch':
                    geopm_args = ['--geopm-record-filter', 'proxy_epoch,all2all']
                elif run_config == 'spin_stride_epoch':
                    geopm_args = ['--geopm-record-filter', 'proxy_epoch,spin,2']

                launcher.run('test_' + cls._test_name, add_geopm_args=geopm_args)


    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_epoch_inference._keep_files = True

    def test_trace_epoch_count(self):
        """
        Test that the EPOCH_COUNT signal reflects the expected count.
        """
        for run_config in self._config_names:
            report_path = 'test_{}_{}.report'.format(self._test_name, run_config)
            raw_report = geopmpy.io.RawReport(report_path)
            node_list = raw_report.host_names()

            profile_trace_path = '{}_profile_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(f'{profile_trace_path}*')

            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                if run_config == 'no_epoch':
                    epoch_count = 0
                else:
                    epoch_count = int(trace[trace['EVENT'] == 'EPOCH_COUNT']['SIGNAL'].iloc[-1])

                if run_config == 'no_epoch':
                    self.assertEqual(0, epoch_count)
                elif run_config == 'spin_epoch':
                    self.assertEqual(20, epoch_count)
                elif run_config in ('barrier_epoch', 'spin_stride_epoch'):
                    self.assertEqual(10, epoch_count)
                else:
                    self.fail("invalid run config: {}".format(run_config))

    def test_trace_epoch_runtime(self):
        """
        Test that the EPOCH_RUNTIME signal reflects the expected runtime.
        """
        for run_config in self._config_names:
            report_path = 'test_{}_{}.report'.format(self._test_name, run_config)
            raw_report = geopmpy.io.RawReport(report_path)
            node_list = raw_report.host_names()

            profile_trace_path = '{}_profile_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(f'{profile_trace_path}*')

            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                runtimes = trace[trace['EVENT'] == 'EPOCH_COUNT']['TIME'].diff()

                # remove nans
                runtimes = runtimes.dropna()
                # get unique values, rounded
                runtimes = set([round(xx, 2) for xx in runtimes])

                if run_config == 'no_epoch':
                    # no non-NAN runtime samples
                    self.assertEqual(set({}), runtimes)
                elif run_config == 'spin_epoch':
                    # runtime flips between two values
                    self.assertEqual({1.5, 2.0}, runtimes)
                elif run_config in ('barrier_epoch', 'spin_stride_epoch'):
                    # consistent runtime
                    self.assertEqual({3.5}, runtimes)
                else:
                    self.fail("invalid run config: {}".format(run_config))

    def test_trace_epoch_runtime_network(self):
        """
        Test that the EPOCH_RUNTIME_NETWORK signal reflects the expected runtime.
        """
        for run_config in self._config_names:
            report_path = 'test_{}_{}.report'.format(self._test_name, run_config)
            raw_report = geopmpy.io.RawReport(report_path)
            node_list = raw_report.host_names()

            trace_path = '{}_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(f'{trace_path}*')

            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                runtimes = []
                epoch_group = trace.groupby('EPOCH_COUNT')
                for ec, group in epoch_group:
                    if group['EPOCH_COUNT'].iloc[-1] == 0:
                        continue
                    GEOPM_REGION_HINT_NETWORK = '0x00000004'
                    if len(group[group['REGION_HINT'] == GEOPM_REGION_HINT_NETWORK]) > 0:
                        first_last = group[group['REGION_HINT'] == GEOPM_REGION_HINT_NETWORK].iloc[[0, -1]]
                        network_runtime = first_last['TIME'].diff().dropna().iloc[0]
                        runtimes += [network_runtime]
                    else:
                        runtimes += [0]

                # get unique values, rounded
                runtimes = set([round(xx, 1) for xx in runtimes])

                if run_config == 'no_epoch':
                    # no non-NAN runtime samples
                    self.assertEqual(set({}), runtimes)
                elif run_config == 'spin_epoch':
                    # runtime flips between two values
                    self.assertEqual({1.0, 0.0}, runtimes)
                elif run_config in ('barrier_epoch', 'spin_stride_epoch'):
                    # consistent network runtime from all2all region
                    self.assertEqual({1.0}, runtimes)
                else:
                    self.fail("invalid run config: {}".format(run_config))

    def test_trace_epoch_runtime_ignore(self):
        """
        Test that the EPOCH_RUNTIME_IGNORE signal reflects the expected runtime.
        """
        for run_config in self._config_names:
            report_path = 'test_{}_{}.report'.format(self._test_name, run_config)
            raw_report = geopmpy.io.RawReport(report_path)
            node_list = raw_report.host_names()

            trace_path = '{}_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(f'{trace_path}*')
            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                runtimes = []
                epoch_group = trace.groupby('EPOCH_COUNT')
                for ec, group in epoch_group:
                    if group['EPOCH_COUNT'].iloc[-1] == 0:
                        continue
                    GEOPM_REGION_HINT_IGNORE = '0x00000008'
                    if len(group[group['REGION_HINT'] == GEOPM_REGION_HINT_IGNORE]) > 0:
                        first_last = group[group['REGION_HINT'] == GEOPM_REGION_HINT_IGNORE].iloc[[0, -1]]
                        network_runtime = first_last['TIME'].diff().dropna().iloc[0]
                        runtimes += [network_runtime]
                    else:
                        runtimes += [0]

                # get unique values, rounded
                runtimes = set([round(xx, 1) for xx in runtimes])

                if run_config == 'no_epoch':
                    # no non-NAN runtime samples
                    self.assertEqual(set({}), runtimes)
                elif run_config in ('barrier_epoch', 'spin_epoch'):
                    # runtime flips between two values
                    self.assertEqual({0.5, 0.0}, runtimes)
                elif run_config == 'spin_stride_epoch':
                    # consistent network runtime from ignore region
                    self.assertEqual({0.5}, runtimes, msg=run_config)
                else:
                    self.fail("invalid run config: {}".format(run_config))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
