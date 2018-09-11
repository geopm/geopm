#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

import os
import sys
import unittest
from collections import defaultdict
from StringIO import StringIO
from analysis_helper import *


class MockAnalysis(geopmpy.analysis.Analysis):
    @staticmethod
    def help_text():
        return 'HELP_TEXT TEST'

    @staticmethod
    def add_options(parser, enforce_required):
        pass

    def launch(self, launch_config):
        print 'LAUNCH'

    def find_files(self):
        print 'FIND_FILES'

    def parse(self):
        print 'PARSE'

    def summary_process(self, parse_output):
        print 'SUMMARY_PROCESS'

    def summary(self, process_output):
        print 'SUMMARY'

    def plot_process(self, parse_output):
        print 'PLOT_PROCESS'

    def plot(self, process_output):
        print 'PLOT'


@unittest.skipIf(g_skip_analysis_test, g_skip_analysis_ex)
class TestAnalysisCommandLine(unittest.TestCase):
    def setUp(self):
        self.old_stdout = sys.stdout
        self.old_stderr = sys.stderr
        sys.stdout = StringIO()
        sys.stderr = StringIO()

        self.old_freq_sweep = geopmpy.analysis.FreqSweepAnalysis
        geopmpy.analysis.FreqSweepAnalysis = MockAnalysis

        self.old_slurm_nnodes = os.getenv('SLURM_NNODES')

    def tearDown(self):
        sys.stdout = self.old_stdout
        sys.stderr = self.old_stderr
        geopmpy.analysis.FreqSweepAnalysis = self.old_freq_sweep
        if self.old_slurm_nnodes:
            os.environ['SLURM_NNODES'] = self.old_slurm_nnodes
        elif 'SLURM_NNODES' in os.environ:
            del os.environ['SLURM_NNODES']

    def test_no_args(self):
        # no args prints error and usage
        rc = geopmpy.analysis.main([])
        self.assertNotEqual(0, rc)
        self.assertIn('Error', sys.stderr.getvalue())
        self.assertIn('Usage', sys.stderr.getvalue())

    def test_bad_type(self):
        with self.assertRaises(SyntaxError) as err:
            geopmpy.analysis.main(['badtype'])

        self.assertIn('Analysis type', str(err.exception))

    def test_help(self):
        rc = geopmpy.analysis.main(['--help'])
        self.assertEqual(0, rc)
        self.assertIn('Usage', sys.stdout.getvalue())

    def test_help_custom(self):
        rc = geopmpy.analysis.main(['freq_sweep', '--help'])
        self.assertEqual(0, rc)
        self.assertIn('Usage', sys.stdout.getvalue())
        self.assertIn('HELP_TEXT TEST', sys.stdout.getvalue())

    def test_launch_missing_param(self):
        with self.assertRaises(RuntimeError) as err:
            geopmpy.analysis.main(['freq_sweep'])
        self.assertIn('--num-rank and --num-node are required for launch', str(err.exception))

    def test_launch_no_alloc(self):
        with self.assertRaises(RuntimeError) as err:
            geopmpy.analysis.main(['freq_sweep', '-N 1', '-n 1'])
        self.assertIn('Launch must be made inside of a job allocation', str(err.exception))

    def test_launch_only(self):
        os.environ['SLURM_NNODES'] = '1'
        rc = geopmpy.analysis.main(['freq_sweep', '-N 1', '-n 1', 'myapp'])
        self.assertEqual(0, rc)
        expected = ['LAUNCH', 'Neither summary nor plot']
        result = sys.stdout.getvalue().strip().split('\n')
        self.assertEqual(len(expected), len(result))
        for exp, res in zip(expected, result):
            self.assertIn(exp, res)

    def test_launch_plot_summary(self):
        os.environ['SLURM_NNODES'] = '1'
        rc = geopmpy.analysis.main(['freq_sweep', '-N 1', '-n 1', '--plot', '--summary', 'myapp'])
        self.assertEqual(0, rc)
        expected = ['LAUNCH', 'FIND_FILES', 'PARSE', 'SUMMARY_PROCESS', 'SUMMARY', 'PLOT_PROCESS', 'PLOT']
        result = sys.stdout.getvalue().strip().split('\n')
        self.assertEqual(len(expected), len(result))
        for exp, res in zip(expected, result):
            self.assertIn(exp, res)

    def test_skip_launch(self):
        rc = geopmpy.analysis.main(['freq_sweep', '--skip-launch', '--plot', '--summary'])
        self.assertEqual(0, rc)
        expected = ['FIND_FILES', 'PARSE', 'SUMMARY_PROCESS', 'SUMMARY', 'PLOT_PROCESS', 'PLOT']
        result = sys.stdout.getvalue().strip().split('\n')
        self.assertEqual(len(expected), len(result))
        for exp, res in zip(expected, result):
            self.assertIn(exp, res)


if __name__ == '__main__':
    unittest.main()
