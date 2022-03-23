#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
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

import unittest
from unittest import mock
import tempfile
import os
import pwd
import grp

from geopmdpy.system_files import ActiveSessions
from geopmdpy.system_files import AccessLists

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.service import PlatformService
    from geopmdpy.service import TopoService

class TestAccessLists(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestAccessLists'
        self._CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        self._access_lists = AccessLists(self._CONFIG_PATH.name)
        self._bad_group_id = 'GEOPM_TEST_INVALID_GROUP_NAME'
        self._bad_user_id = 'GEOPM_TEST_INVALID_USER_NAME'

    def tearDown(self):
        self._CONFIG_PATH.cleanup()

    def _write_group_files_helper(self, group, signals, controls):
        signal_lines = list(signals)
        signal_lines.insert(0, '')
        signal_lines = '\n'.join(signal_lines)

        control_lines = list(controls)
        control_lines.insert(0, '')
        control_lines = '\n'.join(control_lines)

        return signal_lines, control_lines

    def test_config_parser(self):
        '''
        Tests that the parsing of the config files is resilient to
        comments and spacing oddities.
        '''
        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'default', 'power']
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        signal_file = os.path.join(default_dir, 'allowed_signals')
        signal_lines = """# Comment about the file contents
  geopm
energy
\tsignals\t
  # An indented comment
default


"""
        control_file = os.path.join(default_dir, 'allowed_controls')
        control_lines = """controls
\t# Inline comment
geopm


# Another comment
default
  power



"""

        with mock.patch('geopmdpy.pio.signal_names', return_value=signals_expect), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_expect), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[signal_lines, control_lines]) as mock_srf:
            signals, controls = self._access_lists.get_group_access('')
            mock_isdir.assert_called_once_with(default_dir)
            calls = [mock.call(signal_file), mock.call(control_file)]
            mock_srf.assert_has_calls(calls)
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def _set_group_access_test_helper(self, group):
        '''
        Test that the files will be created with the proper data.
        '''
        all_signals = ['power', 'energy']
        all_controls = ['power', 'frequency']
        with mock.patch('geopmdpy.pio.signal_names', return_value=all_signals),  \
             mock.patch('geopmdpy.pio.control_names', return_value=all_controls), \
             mock.patch('geopmdpy.system_files.secure_make_dirs') as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file') as mock_smf:

            self._access_lists.set_group_access(group, ['power'], ['frequency'])

            # Were the files written in the proper order?
            test_dir = os.path.join(self._CONFIG_PATH.name,
                                    '0.DEFAULT_ACCESS' if group == '' else group)

            signal_file = os.path.join(test_dir, 'allowed_signals')
            control_file = os.path.join(test_dir, 'allowed_controls')

            mock_smd.assert_called_once_with(test_dir)
            calls = [mock.call(signal_file, 'power\n'),
                     mock.call(control_file, 'frequency\n')]
            mock_smf.assert_has_calls(calls)

    def test_set_group_access_empty(self):
        self._set_group_access_test_helper('')

    def test_set_group_access_named(self):
        with mock.patch('grp.getgrnam', return_value='named') as mock_getgrnam:
            self._set_group_access_test_helper('named')
            mock_getgrnam.assert_called_once_with('named')

    def test_get_group_access_empty(self):
        signals, controls = self._access_lists.get_group_access('')
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_group_access_invalid(self):
        err_msg = 'Linux group name cannot begin with a digit'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._access_lists.get_group_access('1' + self._bad_group_id)
        err_msg = 'Linux group is not defined'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._access_lists.get_group_access(self._bad_group_id)

    def test_get_group_access_default(self):
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        signal_file = os.path.join(default_dir, 'allowed_signals')
        control_file = os.path.join(default_dir, 'allowed_controls')

        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'default', 'power']
        signal_lines, control_lines = \
            self._write_group_files_helper('', signals_expect, controls_expect)
        with mock.patch('geopmdpy.pio.signal_names', return_value=signals_expect), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_expect), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[signal_lines, control_lines]) as mock_srf:
            signals, controls = self._access_lists.get_group_access('')
            mock_isdir.assert_called_once_with(default_dir)
            calls = [mock.call(signal_file), mock.call(control_file)]
            mock_srf.assert_has_calls(calls)
        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_group_access_named(self):
        named_dir = os.path.join(self._CONFIG_PATH.name, 'named')
        signal_file = os.path.join(named_dir, 'allowed_signals')
        control_file = os.path.join(named_dir, 'allowed_controls')

        signals_expect = ['geopm', 'signals', 'default', 'energy']
        controls_expect = ['geopm', 'controls', 'named', 'power']
        signal_lines, control_lines = self._write_group_files_helper('named', [], controls_expect)

        with mock.patch('geopmdpy.pio.signal_names', return_value=signals_expect), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_expect), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('grp.getgrnam', return_value='named') as mock_getgrnam, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[signal_lines, control_lines]) as mock_srf:
            signals, controls = self._access_lists.get_group_access('named')
            mock_isdir.assert_called_once_with(named_dir)
            calls = [mock.call(signal_file), mock.call(control_file)]
            mock_srf.assert_has_calls(calls)
            mock_getgrnam.assert_called_once_with('named')
        self.assertEqual([], signals)
        self.assertEqual(set(controls_expect), set(controls))

    def test_set_group_access_invalid(self):
        '''
        Test that errors are encountered when there are no
        allowed signals or controls.
        '''
        # Test no pio signals
        signal = ['power']
        err_msg = 'The service does not support any signals that match: "{}"'.format(signal[0])
        with mock.patch('geopmdpy.pio.signal_names', return_value=[]), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            self._access_lists.set_group_access('', signal, [])

        # Test no pio signals or controls
        control = ['power']
        err_msg = 'The service does not support any controls that match: "{}"'.format(control[0])
        with mock.patch('geopmdpy.pio.signal_names', return_value=[]), \
             mock.patch('geopmdpy.pio.control_names', return_value=[]), \
             self.assertRaisesRegex(RuntimeError, err_msg):
            self._access_lists.set_group_access('', [], control)

    def test_get_user_access_empty(self):
        signals, controls = self._access_lists.get_user_access('')
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_user_access_invalid(self):
        bad_user = self._bad_user_id
        err_msg = "Specified user '{}' does not exist.".format(bad_user)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._access_lists.get_user_access(bad_user)

    def test_get_user_groups_invalid(self):
        bad_user = self._bad_user_id
        err_msg = r"getpwnam\(\): name not found: '{}'".format(bad_user)
        with self.assertRaisesRegex(KeyError, err_msg):
            self._access_lists._get_user_groups(bad_user)

        bad_gid = 999999
        fake_pwd = pwd.struct_passwd((None, None, None, bad_gid, None, '', None))
        err_msg = r"getgrgid\(\): gid not found: {}".format(bad_gid)
        with mock.patch('pwd.getpwnam', return_value=fake_pwd), \
             self.assertRaisesRegex(KeyError, err_msg):
            self._access_lists._get_user_groups(bad_user)

    def test_get_user_groups_current(self):
        current_user = pwd.getpwuid(os.getuid()).pw_name
        expected_groups = [grp.getgrgid(gid).gr_name
                           for gid in os.getgrouplist(current_user, os.getgid())]
        groups = self._access_lists._get_user_groups(current_user)
        self.assertEqual(set(expected_groups), set(groups))

    def test_get_user_access_default(self):
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        signal_file = os.path.join(default_dir, 'allowed_signals')
        control_file = os.path.join(default_dir, 'allowed_controls')

        signals_default = ['frequency', 'energy']
        controls_default = ['geopm', 'controls', 'named', 'power']
        signal_lines, control_lines = \
            self._write_group_files_helper('', signals_default, controls_default)

        with mock.patch('geopmdpy.system_files.AccessLists._get_user_groups',
                        return_value=[]), \
             mock.patch('geopmdpy.pio.signal_names', return_value=signals_default), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_default), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[signal_lines, control_lines]) as mock_srf:
            signals, controls = self._access_lists.get_user_access('')
            mock_isdir.assert_called_once_with(default_dir)
            calls = [mock.call(signal_file), mock.call(control_file)]
            mock_srf.assert_has_calls(calls)

        self.assertEqual(set(signals_default), set(signals))
        self.assertEqual(set(controls_default), set(controls))

    def test_get_user_access_valid(self):
        named_dir = os.path.join(self._CONFIG_PATH.name, 'named')
        named_signal_file = os.path.join(named_dir, 'allowed_signals')
        named_control_file = os.path.join(named_dir, 'allowed_controls')

        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        default_signal_file = os.path.join(default_dir, 'allowed_signals')
        default_control_file = os.path.join(default_dir, 'allowed_controls')

        signals_default = ['frequency', 'energy', 'power']
        controls_default = ['frequency', 'power']
        default_signal_lines, default_control_lines = \
            self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature', 'not-available-anymore']
        controls_named = ['frequency_uncore', 'power_uncore', 'not-available-anymore-either']
        named_signal_lines, named_control_lines = \
            self._write_group_files_helper('named', signals_named, controls_named)

        signals_avail = ['frequency', 'energy', 'power', 'temperature', 'extra_power',
                         'extra_temperature']
        controls_avail = ['frequency', 'power', 'frequency_uncore', 'power_uncore',
                          'extra_frequency_uncore', 'extra_power_uncore']

        all_signals = set(signals_named).union(set(signals_default))
        signals_expect = all_signals.intersection(signals_avail)
        all_controls = set(controls_named).union(set(controls_default))
        controls_expect = all_controls.intersection(controls_avail)

        valid_user = 'val'
        groups=['named']
        with mock.patch('geopmdpy.system_files.AccessLists._get_user_groups',
                        return_value=groups), \
             mock.patch('geopmdpy.pio.signal_names', return_value=signals_avail), \
             mock.patch('geopmdpy.pio.control_names', return_value=controls_avail), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('grp.getgrnam', return_value='named') as mock_getgrnam, \
             mock.patch('geopmdpy.system_files.secure_read_file',
                        side_effect=[named_signal_lines, named_control_lines,
                                     default_signal_lines, default_control_lines]) as mock_srf:
            signals, controls = self._access_lists.get_user_access(valid_user)
            calls = [mock.call(named_dir), mock.call(default_dir)]
            mock_isdir.assert_has_calls(calls)
            calls = [mock.call(named_signal_file), mock.call(named_control_file),
                     mock.call(default_signal_file), mock.call(default_control_file)]
            mock_srf.assert_has_calls(calls)
            mock_getgrnam.assert_called_once_with('named')

        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_user_access_root(self):
        signals_default = ['frequency', 'energy', 'power']
        controls_default = ['frequency', 'power']
        self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        self._write_group_files_helper('named', signals_named, controls_named)

        all_signals = signals_default + signals_named
        all_controls = controls_default + controls_named

        with mock.patch('geopmdpy.pio.signal_names', return_value=all_signals), \
             mock.patch('geopmdpy.pio.control_names', return_value=all_controls):
            signals, controls = self._access_lists.get_user_access('root')

        self.assertEqual(set(all_signals), set(signals))
        self.assertEqual(set(all_controls), set(controls))

    def test__read_allowed_invalid(self):
        result = self._access_lists._read_allowed('INVALID_PATH')
        self.assertEqual([], result)


if __name__ == '__main__':
    unittest.main()
