#!/usr/bin/env python3

#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import unittest
import subprocess

class TestBashExamples(unittest.TestCase):
    def setUp(self):
        script_path = os.path.realpath(__file__)
        script_dir = os.path.dirname(script_path)
        all_files = os.listdir(script_dir)
        self.bash_tests = [os.path.join(script_dir, fn) for fn in all_files
                           if (fn.startswith('test_') and fn.endswith('.sh'))]

    def test_all(self):
        for script in self.bash_tests:
            script_name = os.path.basename(script)
            sys.stderr.write(f'\nAbout to run {script_name} ...\n')
            with self.subTest(bash_example=script_name):
                pid = subprocess.Popen([script],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       universal_newlines=True)
                out, err = pid.communicate()
                output = f'\n\nSTDOUT:\n{out}\n\nSTDERR:\n{err}'
                self.assertEqual(0, pid.returncode, output)
                sys.stdout.write(f'{script_name}{output}\n\nSUCCESS\n')


if __name__ == '__main__':
    unittest.main()
