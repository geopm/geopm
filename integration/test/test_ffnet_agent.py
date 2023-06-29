#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the ffnet agent is functional.
"""
import json
import sys
import unittest
import os

import geopmpy.agent
import geopmpy.io
import geopmdpy.error

from integration.test import util
from integration.test import geopm_test_launcher
from experiment.ffnet import ffnet

@util.skip_unless_config_enable('beta')
@util.skip_unless_do_launch()
@util.skip_unless_workload_exists("apps/arithmetic_intensity/ARITHMETIC_INTENSITY/bench_sse")

class TestIntegration_ffnet(unittest.TestCas):
    @classmethod
    def setUpClass(cls):
        """
        Setup applications, execute, and set up class variables.
        """
    def test_ffnet_agent(self):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_ffnet'
        cls._report_path = '{}.report'.format(test_name)
        cls._trace_path = '{}.trace'.format(test_name)
        cls._agent_conf_path = 'test_' + test_name + '-agent-config.json'

        # Set the job size parameters
        cls._num_node = 1
        num_rank = 2
        time_limit = 6000

        # Configure the test application
        cls._spin_bigo = 0.5
        cls._sleep_bigo = 1.0
        cls._unmarked_bigo = 1.0
        cls._dgemm_bigo = 1.0
        cls._stream_bigo = 2.0
        cls._loop_count = 5

        app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
        app_conf.set_loop_count(cls._loop_count)
        app_conf.append_region('spin', cls._spin_bigo)
        app_conf.append_region('sleep', cls._sleep_bigo)
        app_conf.append_region('sleep-unmarked', cls._unmarked_bigo)
        app_conf.append_region('dgemm', cls._dgemm_bigo)
        app_conf.append_region('stream', cls._stream_bigo)
        app_conf.write()

        # Configure the ffnet agent
        cls._ffnet_policy = {'perf_energy_bias':0}
        cls._agent = 'ffnet'
        cpu_nn_path = os.path.dirname(__file__) + "/ffnet_dummy.json"
        cpu_fmap_path = os.path.dirname(__file__) + "/fmap_dummy.json"

        os.environ["GEOPM_CPU_NN_PATH"] = cpu_nn_path
        os.environ["GEOPM_CPU_FMAP_PATH"] = cpu_fmap_path

        agent_conf = geopmpy.agent.AgentConf(test_name + '_agent.config')

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf=app_conf,
                                                    agent_conf=agent_conf,
                                                    report_path=cls._report_path,
                                                    trace_path=cls._trace_path,
                                                    trace_signals=trace_signals)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)

        # Run the ffnet agent with dummy neural net and fmap
        launcher.run(cls._test_name)

        #Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._trace = geopmpy.io.AppOutput('{}*'.format(cls._trace_path))

    def test_agent_ffnet_dummy_expected_region(self):
        """
        Testing to ensure that the correct region is identified with dummy
        neural net that maps REGION_HASH to region.
        """

    def test_agent_ffnet_dummy_expected_fmap(self):
        """
        Testing to ensure that the correct frequency is set based on the
        identified region.
        """
        #TODO: Figure out how to not hardcode this--read in fmap_dummy
        #      and determine how to get region hashes from reports
        expected_freqs={"dgemm":1.2e9, "stream":2.1e9, "spin":1.6e9, "sleep":1e9}

        host_names = self._report.host_names()
        self.assertEqual(len(host_names), self._num_node)
        for host in host_names:
            for region_name in self._report.region_names(host):
                raw_region = self._report.raw_region(host_name=host,
                                                     region_name=region_name)
                if (region_name in expected_freqs.keys()):
                    #TODO: Figure out how to get freq from this object


#TODO [stretch]: Run geopmbench on pkg0 with dgemm/sleep, geopmbench on pkg1 with stream
#TODO:           and check that apps are identified independently on each socket

if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
