#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
                               {'CPU_POWER_LIMIT_TOTAL': 150})
        # default value policy
        self.check_json_output(['--agent', 'power_governor', '--policy', 'NAN'],
                               {'CPU_POWER_LIMIT_TOTAL': 'NAN'})
        self.check_json_output(['--agent', 'power_governor', '--policy', 'nan'],
                               {'CPU_POWER_LIMIT_TOTAL': 'NAN'})
        # unspecified policy values are accepted
        self.check_json_output(['--agent', 'power_balancer', '--policy', '150'],
                               {'CPU_POWER_LIMIT_TOTAL': 150})
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
