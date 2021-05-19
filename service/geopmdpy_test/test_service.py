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

import os
import unittest
import tempfile
from geopmdpy.service import PlatformService

class MockPlatformIO(object):
    pass

class TestPlatformService(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestPlatformService'
        self._mock_pio = MockPlatformIO()
        self._CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        self._VAR_PATH = tempfile.TemporaryDirectory('{}_var'.format(self._test_name))
        self._platform_service = PlatformService()
        self._platform_service._pio = self._mock_pio
        self._platform_service._CONFIG_PATH = self._CONFIG_PATH.name
        self._platform_service._VAR_PATH = self._VAR_PATH.name
        self._platform_service._ALL_GROUPS = ['named']

    def tearDown(self):
        self._CONFIG_PATH.cleanup()
        self._VAR_PATH.cleanup()

    def test_get_group_access_empty(self):
        signals, controls = self._platform_service.get_group_access('')
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_group_access_default(self):
        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'default', 'power']
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        os.makedirs(default_dir)
        signal_file = os.path.join(default_dir, 'allowed_signals')
        lines = """# Comment about the file contents
  geopm
energy
\tsignals\t
  # An indented comment
default


"""
        with open(signal_file, 'w') as fid:
            fid.write(lines)
        control_file = os.path.join(default_dir, 'allowed_controls')
        lines = """controls
\t# Inline comment
geopm


# Another comment
default
  power



"""
        with open(control_file, 'w') as fid:
            fid.write(lines)
        signals, controls = self._platform_service.get_group_access('')
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_group_access_named(self):
        controls_expect = ['geopm', 'controls', 'named', 'power']
        named_dir = os.path.join(self._CONFIG_PATH.name, 'named')
        os.makedirs(named_dir)
        control_file = os.path.join(named_dir, 'allowed_controls')
        lines = list(controls_expect)
        lines.insert(0, '')
        lines = '\n'.join(lines)
        with open(control_file, 'w') as fid:
            fid.write(lines)
        with self.assertRaises(RuntimeError) as context:
            self._platform_service.get_group_access('INVALID_GROUP_NAME')
        signals, controls = self._platform_service.get_group_access('named')
        self.assertEqual([], signals)
        self.assertEqual(set(controls_expect), set(controls))

if __name__ == '__main__':
    unittest.main()
