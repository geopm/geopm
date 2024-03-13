#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
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

class TestIntegration_ffnet(unittest.TestCase):
    @classmethod
    def setUpClass(cls):  #DELME Run exactly once for everything you want to test
        """
        Setup applications, execute, and set up class variables.
        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'test_ffnet'
        cls._report_path = '{}.report'.format(cls._test_name)
        cls._trace_path = '{}.trace'.format(cls._test_name)
        cls._agent_conf_path = 'test_' + cls._test_name + '-agent-config.json'

        # Set the job size parameters
        cls._num_node = 1
        num_rank = 2
        time_limit = 6000

        # Configure the test application
        test_app_params = {
            'spin_bigo': 0.5,
            'sleep_bigo': 1.0,
            'sleep-unmarked_bigo': 1.0,
            'dgemm_bigo': 1.0,
            'stream_bigo': 2.0,
            'loop_count': 5
        }

        app_conf = geopmpy.io.BenchConf(cls._test_name + '_app.config')
        app_conf.set_loop_count(test_app_params['loop_count'])
        for region in ['spin', 'sleep', 'sleep-unmarked', 'dgemm', 'stream']:
            app_conf.append_region(region, test_app_params[f"{region}_bigo"])
        app_conf.write()

        # Configure the ffnet agent
        cls._ffnet_policy = {'PERF_ENERGY_BIAS':0}
        cls._agent = 'ffnet'
        cls._cpu_nn_path = os.path.dirname(__file__) + "/ffnet_dummy.json"
        cls._cpu_fmap_path = os.path.dirname(__file__) + "/fmap_dummy.json"

        os.environ["GEOPM_CPU_NN_PATH"] = cls._cpu_nn_path
        os.environ["GEOPM_CPU_FMAP_PATH"] = cls._cpu_fmap_path

        agent_conf = geopmpy.agent.AgentConf(cls._test_name + '_agent.config',
                                             cls._agent, cls._ffnet_policy)
        trace_signals = 'REGION_HASH@package' #,geopmbench-0x536c798f_cpu_0@package,geopmbench-0x644f9787_cpu_0@package,geopmbench-0x725e8066_cpu_0@package,geopmbench-0xa74bbf35_cpu_0@package,geopmbench-0xd691da00_cpu_0@package,geopmbench-0xeebad5f7_cpu_0@package'

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

#    def test_agent_ffnet_dummy_expected_region(self):
#        """
#        Testing to ensure that the correct region is identified with dummy
#        neural net that maps REGION_HASH to region. Criteria is >=95%
#        confidence >=95% of the time (PAC)
#        """
#     #   import pdb; pdb.set_trace()
#
#        fmap = json.load(open(cli._cpu_fmap_path))
#        for region_key in fmap:
#            region_hash = region_key.str("-")[1]
#            region_rows = cls._trace["REGION_HASH"] = region_hash
#
#            #TODO: How to assert >=
#            util.assertNear(self, 0.95, region_confidence, msg=msg)
#


    def test_agent_ffnet_dummy_expected_fmap(self):
        """
        Testing to ensure that the correct frequency is set based on the
        identified region.
        """
        fmap = json.load(open(self._cpu_fmap_path))

        host_names = self._report.host_names()
        self.assertEqual(len(host_names), self._num_node)
        for host in host_names:
            for region_name in self._report.region_names(host):
                raw_region = self._report.raw_region(host_name=host,
                                                     region_name=region_name)
                region_hash = raw_region['hash']
                region_key = f"geopmbench-0x{region_hash:08x}"
                if region_key in fmap:
                    msg = region_name + ": frequency should be near assigned map frequency"
                    actual = raw_region['frequency (Hz)']
                    expect = fmap[region_key]
                    util.assertNear(self, expect, actual, msg=msg)

#TODO: Run geopmbench on pkg0, ffnet phi=0 and phi=1
#TODO:     and check that perf is within 2% for phi=0
#TODO:     and check perf @phi0 > perf @phi1
#TODO:     and check total energy@phi0 > total energy@phi1

#TODO: If on system with gpu, run parres dgemm ffnet with phi=0 and phi=1
#TODO:     and check that perf is within 2% for phi=0
#TODO:     and check perf0 > perf1
#TODO:     and check gpu energy0 > gpu energy1
#TODO:     and check that other gpus are not steered

#TODO [stretch]: Run geopmbench on pkg0 with dgemm/sleep, geopmbench on pkg1 with stream
#TODO:           and check that apps are identified independently on each socket

#Get the set of region probabilities given pandas trace lines
#def region_probabilities(trace_lines):
#    nn = json.load(open(cli._cpu_nn_path))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
