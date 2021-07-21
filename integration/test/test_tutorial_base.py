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

"""TUTORIAL_BASE

Run through the base tutorial as a user would.

"""

import sys
import unittest
import os
import subprocess
import shutil

# Put integration test directory into the path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
try:
    # Try to load geopmpy without modifiying the path
    import geopmpy.io
    import geopmpy.error
    import geopmpy.hash
except ImportError:
    # If geopmpy is not installed in the PYTHONPATH then add local
    # copy to path
    import geopm_context
    import geopmpy.io
    import geopmpy.error
    import geopmpy.hash
import util


class TestIntegration_tutorial_base(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'tutorial_base'
        cls._skip_launch = not util.do_launch()
        cls._script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._base_dir = os.path.dirname(os.path.dirname(cls._script_dir))
        cls._tmp_link = os.path.join(cls._script_dir, 'test_tutorial_base')
        # Work around to avoid building on compute nodes:
        #   Check if the test setup script has been run more recently
        #   than the geopm library build.
        do_build = True
        try:
            lib_path = os.path.join(cls._base_dir, '.libs', 'libgeopm.so')
            if os.stat(cls._tmp_link).st_mtime > os.stat(lib_path).st_mtime:
                do_build = False
        except:
            pass
        if do_build:
            build_script = os.path.join(cls._script_dir, 'test_tutorial_base.sh')
            try:
                subprocess.check_call(build_script, shell=True)
            except subprocess.CalledProcessError as ex:
                raise RuntimeError('Failed to build the tutorials, try running "{}" on a system with all compilation requirements prior to running this test.\n    {}'.format(build_script, ex.output))

        if not cls._skip_launch:
            cls.launch()


    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_tutorial_base._keep_files = True

    @classmethod
    def launch(cls):
        "Run the tutorial scripts"
        run_script = '''set -e
                        cd {tmp_link}/geopm-tutorial
                        ./tutorial_0.sh
                        ./tutorial_1.sh
                        ./tutorial_2.sh
                        ./tutorial_3.sh
                        ./tutorial_4.sh
                        ./tutorial_5.sh
                        ./tutorial_6.sh'''.format(tmp_link=cls._tmp_link)
        subprocess.check_call(run_script, shell=True)

    @util.skip_unless_do_launch()
    def test_generate_reports(self):
        "Check that reports were generated"
        expected_reports = ['tutorial_0_report',
                            'tutorial_1_report',
                            'tutorial_2_report',
                            'tutorial_3_balanced_report',
                            'tutorial_3_governed_report',
                            'tutorial_4_balanced_report',
                            'tutorial_4_governed_report',
                            'tutorial_5_report',
                            'tutorial_6_report']
        out_dir = '{tmp_link}/geopm-tutorial'.format(tmp_link=self._tmp_link)
        for report in expected_reports:
            report_path = os.path.join(out_dir, report)
            self.assertTrue(os.path.exists(report_path))


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
