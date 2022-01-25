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
import subprocess
import json

import geopmpy.io
import geopmpy.hash


class TestIntegrationGeopmagent(unittest.TestCase):
    ''' Tests of geopmagent.'''
    def setUp(self):
        self.exec_name = 'geopmagent'
        self.skip_warning_string = 'Incompatible CPU frequency driver/governor'

    def check_output(self, args, expected):
        try:
            with subprocess.Popen([self.exec_name] + args,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
                proc.wait()
                for exp in expected:
                    line = proc.stdout.readline()
                    while self.skip_warning_string.encode() in line or line == b'\n':
                        line = proc.stdout.readline()
                    self.assertIn(exp.encode(), line)
                for line in proc.stdout:
                    if self.skip_warning_string.encode() not in line:
                        self.assertNotIn(b'Error', line)
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{}\n'.format(ex.output))

    def check_json_output(self, args, expected):
        try:
            with subprocess.Popen([self.exec_name] + args,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
                proc.wait()
                line = proc.stdout.readline()
                while self.skip_warning_string.encode() in line or line == b'\n':
                    line = proc.stdout.readline()
                try:
                    out_json = json.loads(line.decode())
                except ValueError:
                    self.fail('Could not convert json string: {}\n'.format(line))
                self.assertEqual(expected, out_json)
                for line in proc.stdout:
                    if self.skip_warning_string.encode() not in line:
                        self.assertNotIn(b'Error', line)
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{}\n'.format(ex.output))

    def check_no_error(self, args):
        try:
            with subprocess.Popen([self.exec_name] + args,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
                proc.wait()
                for line in proc.stdout:
                    if self.skip_warning_string.encode() not in line:
                        self.assertNotIn(b'Error', line)
                proc.stdout.close()
        except subprocess.CalledProcessError as ex:
            sys.stderr.write('{}\n'.format(ex.output))

    def test_geopmagent_command_line(self):
        '''
        Check that geopmagent commandline arguments work.
        '''
        # no args
        agent_names = ['monitor', 'power_balancer', 'power_governor',
                       'frequency_map']
        self.check_output([], agent_names)

        # help message
        self.check_output(['--help'], ['Usage'])

        # version
        self.check_no_error(['--version'])

        # agent policy and sample names
        for agent in agent_names:
            self.check_output(['--agent', agent],
                              ['Policy', 'Sample'])

        # policy file
        self.check_json_output(['--agent', 'monitor', '--policy', 'None'],
                               {})
        self.check_json_output(['--agent', 'power_governor', '--policy', '150'],
                               {'POWER_PACKAGE_LIMIT_TOTAL': 150})
        # default value policy
        self.check_json_output(['--agent', 'power_governor', '--policy', 'NAN'],
                               {'POWER_PACKAGE_LIMIT_TOTAL': 'NAN'})
        self.check_json_output(['--agent', 'power_governor', '--policy', 'nan'],
                               {'POWER_PACKAGE_LIMIT_TOTAL': 'NAN'})
        # unspecified policy values are accepted
        self.check_json_output(['--agent', 'power_balancer', '--policy', '150'],
                               {'POWER_PACKAGE_LIMIT_TOTAL': 150})
        # hashing works for frequency map agent
        self.check_json_output(['--agent', 'frequency_map', '--policy', '1e9,nan,hello,2e9'],
                               {'FREQ_DEFAULT': 1e9, 'FREQ_UNCORE': 'NAN',
                                'HASH_0': geopmpy.hash.crc32_str('hello'), 'FREQ_0': 2e9})
        # errors
        self.check_output(['--agent', 'power_governor', '--policy', 'None'],
                          ['not a valid floating-point number', 'Invalid argument'])
        self.check_output(['--agent', 'monitor', '--policy', '300'],
                          ['agent takes no parameters', 'Invalid argument'])


if __name__ == '__main__':
    unittest.main()
