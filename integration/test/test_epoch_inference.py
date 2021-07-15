#!/usr/bin/env python3
#
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

import geopm_context
import geopmpy.io
import geopmpy.agent
import geopmpy.error

import util
if util.do_launch():
    # Note: this import may be moved outside of do_launch if needed to run
    # commands on compute nodes such as geopm_test_launcher.geopmread
    import geopm_test_launcher


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
                cls._files.append(report_path)

                # Create the test launcher with the above configuration
                trace_signals = "EPOCH_RUNTIME,EPOCH_RUNTIME_IGNORE,EPOCH_RUNTIME_NETWORK"
                launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                            agent_conf,
                                                            report_path,
                                                            trace_path,
                                                            time_limit=time_limit,
                                                            trace_signals=trace_signals)
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
            trace_path = '{}_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(report_path, trace_path+'*')
            node_list = output.get_node_names()
            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                last_row = trace.iloc[-1]
                sys.stdout.write("{}: epoch_count={}\n".format(run_config, last_row["EPOCH_COUNT"]))
                epoch_count = last_row["EPOCH_COUNT"]
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
            trace_path = '{}_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(report_path, trace_path+'*')
            node_list = output.get_node_names()
            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                runtimes = trace["EPOCH_RUNTIME"]
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
            trace_path = '{}_{}'.format(self._trace_path_prefix, run_config)
            output = geopmpy.io.AppOutput(report_path, trace_path+'*')
            node_list = output.get_node_names()
            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                runtimes = trace["EPOCH_RUNTIME_NETWORK"]
                # remove nans
                runtimes = runtimes.dropna()
                # get unique values, rounded
                runtimes = set([round(xx, 2) for xx in runtimes])

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
            trace_path = '{}_{}'.format(self._trace_path_prefix, run_config)

            output = geopmpy.io.AppOutput(report_path, trace_path+'*')
            node_list = output.get_node_names()
            for node in node_list:
                trace = output.get_trace_data(node_name=node)
                runtimes = trace["EPOCH_RUNTIME_IGNORE"]
                # remove nans
                runtimes = runtimes.dropna()
                # get unique values, rounded
                runtimes = set([round(xx, 2) for xx in runtimes])

                if run_config == 'no_epoch':
                    # no non-NAN runtime samples
                    self.assertEqual(set({}), runtimes)
                elif run_config == 'spin_epoch':
                    # runtime flips between two values
                    self.assertEqual({0.5, 0.0}, runtimes)
                elif run_config in ('barrier_epoch', 'spin_stride_epoch'):
                    # consistent network runtime from ignore region
                    self.assertEqual({0.5}, runtimes)
                else:
                    self.fail("invalid run config: {}".format(run_config))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
