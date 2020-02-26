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

"""EE_SHORT_REGION_SLOP

"""

import sys
import unittest
import os
import glob

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_context
import geopmpy.io
from test_integration import geopm_test_launcher
from test_integration import util

_g_skip_launch = False

class AppConf(object):
    """Class that is used by the test launcher in place of a
    geopmpy.io.BenchConf when running the ee_short_region_slop benchmark.

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
        return os.path.join(script_dir, '.libs', 'test_ee_short_region_slop')

    def get_exec_args(self):
        """Returns a list of strings representing the command line arguments
        to pass to the test-application for the next run.  This is
        especially useful for tests that execute the test-application
        multiple times.

        """
        return []


class TestIntegration_ee_short_region_slop(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Create launcher, execute benchmark and set up class variables.

        """
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        test_name = 'ee_short_region_slop'
        cls._report_path = test_name + '.report'
        cls._trace_path = test_name + '.trace'
        cls._skip_launch = _g_skip_launch
        cls._keep_files = os.getenv('GEOPM_KEEP_FILES') is not None
        cls._agent_conf_path = test_name + '-agent-config.json'
        # Clear out exception record for python 2 support
        if 'exc_clear' in dir(sys):
            sys.exc_clear()
        if not cls._skip_launch:
            # Set the job size parameters
            num_node = 1
            num_rank = 1
            time_limit = 6000
            # Configure the test application
            app_conf = AppConf()
            # Configure the agent
            # Query for the min and sticker frequency and run the
            # energy efficient agent over this range.
            freq_min = geopm_test_launcher.geopmread("CPUINFO::FREQ_MIN board 0")
            freq_sticker = geopm_test_launcher.geopmread("CPUINFO::FREQ_STICKER board 0")
            agent_conf_dict = {'FREQ_MIN':freq_min,
                               'FREQ_MAX':freq_sticker}
            agent_conf = geopmpy.io.AgentConf(cls._agent_conf_path,
                                              'energy_efficient',
                                              agent_conf_dict)
            # Create the test launcher with the above configuration
            launcher = geopm_test_launcher.TestLauncher(app_conf,
                                                        agent_conf,
                                                        cls._report_path,
                                                        cls._trace_path,
                                                        time_limit=time_limit)
            launcher.set_num_node(num_node)
            launcher.set_num_rank(num_rank)
            # Run the test application
            launcher.run(test_name)

    @classmethod
    def tearDownClass(cls):
        """Clean up any files that may have been created during the test if we
        are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset.

        """

        if (sys.exc_info() == (None, None, None) and not
            cls._keep_files and not
            cls._skip_launch):
            os.unlink(cls._agent_conf_path)
            os.unlink(cls._report_path)
            for tf in glob.glob(cls._trace_path + '.*'):
                os.unlink(tf)

    def test_load_report(self):
        """Test that the report can be loaded

        """
        report = geopmpy.io.RawReport(self._report_path)

if __name__ == '__main__':
    try:
        sys.argv.remove('--skip-launch')
        _g_skip_launch = True
    except ValueError:
        if 'exc_clear' in dir(sys):
            sys.exc_clear()
    unittest.main()
