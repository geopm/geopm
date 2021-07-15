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
import sys
import unittest
import os


from integration.test import geopm_context
import geopmpy.io
import geopmpy.error
import util
import geopm_test_launcher


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
