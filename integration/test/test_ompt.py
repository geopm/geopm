#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import sys
import unittest
import os

import geopmpy.io
import geopmpy.agent
import geopmdpy.error
from integration.test import util
from integration.test import geopm_test_launcher


@util.skip_unless_config_enable('ompt')
class TestIntegration_ompt(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'test_ompt'
        cls._report_path = '{}.report'.format(test_name)
        cls._agent_conf_path = test_name + '-agent-config.json'
        # Set the job size parameters
        cls._num_node = 4
        num_rank = 16

        app_conf = geopmpy.io.BenchConf(test_name + '_app.config')
        app_conf.append_region('stream-unmarked', 1.0)
        agent_conf = geopmpy.agent.AgentConf(test_name + '_agent.config')

        # Create the test launcher with the above configuration
        launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                    agent_conf,
                                                    cls._report_path)
        launcher.set_num_node(cls._num_node)
        launcher.set_num_rank(num_rank)
        # Run the test application
        launcher.run(test_name)

        # Output to be reused by all tests
        cls._report = geopmpy.io.RawReport(cls._report_path)
        cls._node_names = cls._report.host_names()

    def test_unmarked_ompt(self):
        report = geopmpy.io.RawReport(self._report_path)

        node_names = report.host_names()
        self.assertEqual(len(node_names), self._num_node)
        stream_id = None
        for nn in node_names:
            region_names = report.region_names(nn)
            stream_name = [key for key in region_names if key.lower().find('stream') != -1][0]
            stream_data = report.raw_region(host_name=nn, region_name=stream_name)
            found = False
            for name in region_names:
                if stream_name in name:  # account for numbers at end of OMPT region names
                    found = True
            self.assertTrue(found)
            self.assertLessEqual(1, stream_data['count'])
            if stream_id:
                self.assertEqual(stream_id, stream_data['hash'])
            else:
                stream_id = stream_data['hash']
            ompt_regions = [key for key in region_names if key.startswith('[OMPT]')]
            self.assertLessEqual(1, len(ompt_regions))

if __name__ == '__main__':
    unittest.main()
