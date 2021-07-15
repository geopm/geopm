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

"""FREQUENCY_HINT_USAGE

This integration test verifies that the hint associated with a user-marked
region has the desired effect on the agent's chosen frequency for that region
based on the agent behavior discussed in the corresponding man pages.

"""

import sys
import unittest
import os
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import geopm_context
import geopmpy.io
import geopmpy.agent
import geopmpy.error

import util
import geopm_test_launcher


class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the frequency_hint_usage benchmark.

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
        return os.path.join(script_dir, '.libs', 'test_frequency_hint_usage')

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


@util.skip_unless_do_launch()
class TestIntegration_frequency_hint_usage(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'frequency_hint_usage'
        cls._skip_launch = not util.do_launch()

        cls._fmap_report_path = 'test_{}_fmap.report'.format(test_name)
        cls._fmap_trace_path = 'test_{}_fmap.trace'.format(test_name)
        cls._fmap_agent_conf_path = 'test_' + test_name + '-fmap-agent-config.json'

        cls._ee_report_path = 'test_{}_ee.report'.format(test_name)
        cls._ee_trace_path = 'test_{}_ee.trace'.format(test_name)
        cls._ee_agent_conf_path = 'test_' + test_name + '-ee-agent-config.json'

        cls._freq_min = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
        cls._freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
        cls._freq_step = geopm_test_launcher.geopmread("FREQUENCY_STEP board 0")
        cls._freq_default = cls._freq_sticker - cls._freq_step

        if not cls._skip_launch:
            # Set the job size parameters
            num_node = 1
            num_rank = 1
            time_limit = 6000
            # Configure the test application
            app_conf = AppConf()

            # Configure the agents
            agent_conf_dict = {'FREQ_DEFAULT': cls._freq_default}
            fmap_agent_conf = geopmpy.agent.AgentConf(cls._fmap_agent_conf_path,
                                                      'frequency_map',
                                                      agent_conf_dict)
            agent_conf_dict = {'FREQ_MIN': cls._freq_min,
                               'FREQ_MAX': cls._freq_default}
            ee_agent_conf = geopmpy.agent.AgentConf(cls._ee_agent_conf_path,
                                                    'energy_efficient',
                                                    agent_conf_dict)

            # Fmap run
            launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                        fmap_agent_conf,
                                                        cls._fmap_report_path,
                                                        cls._fmap_trace_path,
                                                        time_limit=time_limit)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            launcher.run('test_' + test_name)

            # EE run - Not available; Removed from Agent factory.
            #  launcher = geopm_test_launcher.TestLauncher(app_conf,
            #                                              ee_agent_conf,
            #                                              cls._ee_report_path,
            #                                              cls._ee_trace_path,
            #                                              time_limit=time_limit)
            #  launcher.set_num_node(num_node)
            #  launcher.set_num_rank(num_rank)
            #  launcher.run(test_name)


    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_frequency_hint_usage._keep_files = True

    def test_frequency_sane_fmap(self):
        """
        Test that the hint is respected for MPI regions within a
        user marked region with the compute hint.
        """
        report = geopmpy.io.RawReport(self._fmap_report_path)
        host_name = report.host_names()[0]

        region = report.raw_region(host_name, 'compute_region')
        assigned_freq = report.get_field(region, 'frequency-map')
        achieved_freq = report.get_field(region, 'frequency', 'Hz')

        # Fmap agent should assign policy default frequency for regions not
        # in the map.
        self.assertEqual(self._freq_default, assigned_freq,
                         msg='Expected assigned frequency to be max from policy since this region has the COMPUTE hint. ({} != {})'.format(self._freq_default, assigned_freq))
        util.assertNear(self, self._freq_default, achieved_freq,
                        msg='Expected achieved frequency to be near default from policy. ({} !~= {})'.format(self._freq_default, achieved_freq))

    @unittest.skip('Disabled pending overhaul of agent.')
    def test_frequency_sane_ee(self):
        """
        Test that the hint is respected for MPI regions within a
        user marked region with the compute hint.
        """
        report = geopmpy.io.RawReport(self._ee_report_path)
        host_name = report.host_names()[0]

        region = report.raw_region(host_name, 'compute_region')
        try:
            assigned_freq = report.get_field(region, 'requested-online-frequency')
        except KeyError:
            self.fail('Expected field "requested-online-frequency" not in region.')
        achieved_freq = report.get_field(region, 'frequency', 'Hz')

        # EE agent should assign not the minimum frequency for regions
        # with the COMPUTE hint.
        self.assertNotEqual(self._freq_min, assigned_freq,
                            msg='Expected assigned frequency to not be the min from policy since this region has the COMPUTE hint. ({} == {}).'.format(self._freq_min, assigned_freq))
        util.assertNotNear(self, self._freq_min, achieved_freq, epsilon=0.5,
                           msg='Expected achieved frequency to NOT be near min from policy. ({} ~= {})'.format(self._freq_min, achieved_freq))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
