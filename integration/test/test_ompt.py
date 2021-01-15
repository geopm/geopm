#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

import geopm_context
import geopmpy.io

import util


class TestIntegration_ompt(unittest.TestCase):
    @util.skip_unless_config_enable('ompt')
    def test_unmarked_ompt(self):
        name = 'test_unmarked_ompt'
        report_path = name + '.report'
        num_node = 4
        num_rank = 16
        app_conf = geopmpy.io.BenchConf(name + '_app.config')
        self._tmp_files.append(app_conf.get_path())
        app_conf.append_region('stream-unmarked', 1.0)
        app_conf.append_region('dgemm-unmarked', 1.0)
        app_conf.append_region('all2all-unmarked', 1.0)
        agent_conf = geopmpy.io.AgentConf(name + '_agent.config', self._agent, self._options)
        self._tmp_files.append(agent_conf.get_path())
        launcher = geopm_test_launcher.TestLauncher(app_conf, agent_conf, report_path)
        launcher.set_num_node(num_node)
        launcher.set_num_rank(num_rank)
        launcher.run(name)

        self._output = geopmpy.io.AppOutput(report_path)
        node_names = self._output.get_node_names()
        self.assertEqual(len(node_names), num_node)
        stream_id = None
        region_names = self._output.get_region_names()
        stream_name = [key for key in region_names if key.lower().find('stream') != -1][0]
        for nn in node_names:
            stream_data = self._output.get_report_data(node_name=nn, region=stream_name)
            found = False
            for name in region_names:
                if stream_name in name:  # account for numbers at end of OMPT region names
                    found = True
            self.assertTrue(found)
            self.assertEqual(1, stream_data['count'].item())
            if stream_id:
                self.assertEqual(stream_id, stream_data['id'].item())
            else:
                stream_id = stream_data['id'].item()
            ompt_regions = [key for key in region_names if key.startswith('[OMPT]')]
            self.assertLessEqual(2, len(ompt_regions))
            self.assertTrue(('MPI_Alltoall' in region_names))
            gemm_region = [key for key in region_names if key.lower().find('gemm') != -1]
            self.assertLessEqual(1, len(gemm_region))



if __name__ == '__main__':
    unittest.main()
