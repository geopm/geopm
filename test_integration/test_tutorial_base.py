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

"""TUTORIAL_BASE

Run through the base tutorial as a user would.

"""

import sys
import unittest
import os
import subprocess

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
    from test_integration import geopm_context
    import geopmpy.io
    import geopmpy.error
    import geopmpy.hash

_g_skip_launch = False
try:
    # Check for skip launch command line arguement
    sys.argv.remove('--skip-launch')
    _g_skip_launch = True
except ValueError:
    # If we are not skipping the launch we need to import the test
    # launcher
    from test_integration import geopm_test_launcher
    from test_integration import util
    geopmpy.error.exc_clear()


class TestIntegration_tutorial_base(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')
        cls._test_name = 'tutorial_base'
        cls._skip_launch = _g_skip_launch
        cls._keep_files = cls._skip_launch or os.getenv('GEOPM_KEEP_FILES') is not None
        cls._script_dir = os.path.dirname(os.path.realpath(__file__))
        cls._base_dir = os.path.dirname(cls._script_dir)
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
            subprocess.check_call(build_script, shell=True)

        # Clear out exception record for python 2 support
        geopmpy.error.exc_clear()
        if not cls._skip_launch:
            cls.launch()


    @classmethod
    def tearDownClass(cls):
        """Clean up any files that may have been created during the test if we
        are not handling an exception and the GEOPM_KEEP_FILES
        environment variable is unset.

        """
        if (sys.exc_info() == (None, None, None) and not
            cls._keep_files):
            tmp_dir = os.readlink(cls._tmp_link)
            os.rmtree(tmp_dir)
            os.unlink(cls._tmp_link)


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


    def test_generate_reports(self):
        "Check that reports were generated"
        for tutorial_idx in range(7):
            self.assrtTrue(os.path.exists('{tmp_link}/geopm-tutorial/tutorial_{tutorial_idx}.report'.format(
                                              tmp_link=self.tmp_link, tutorial_idx=tutorial_idx)))


if __name__ == '__main__':
    unittest.main()
