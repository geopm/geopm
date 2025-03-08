#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
import tempfile
import os
import stat
import pwd
import grp
import shutil

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.system_files import ActiveSessions
    from geopmdpy.system_files import AccessLists
    from geopmdpy.system_files import _get_names
    from geopmdpy.service import PlatformService
    from geopmdpy.service import TopoService
    from geopmdpy import pio

class TestAccessLists(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestAccessLists'
        self._CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        self._signals_expect = ['controls', 'default', 'energy', 'geopm', 'power', 'signals']
        self._controls_expect = ['controls', 'default', 'geopm', 'power']

        with mock.patch('geopmdpy.system_files._get_names',
                        return_value=(self._signals_expect, self._controls_expect)):
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

    def test_get_names(self):
        self.assertEqual((pio.signal_names(), pio.control_names()), _get_names())

    def test_config_parser(self):
        '''
        Tests that the parsing of the config files is resilient to
        comments and spacing oddities.
        '''
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

        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(self._signals_expect, self._controls_expect)), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[control_lines, signal_lines]) as mock_srf:
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            signals, controls = self._access_lists.get_group_access('')
            mock_isdir.assert_called_with(default_dir)
            calls = [mock.call(control_file), mock.call(signal_file)]
            mock_srf.assert_has_calls(calls)
        self.assertEqual(set(self._signals_expect), set(signals))
        self.assertEqual(set(self._controls_expect), set(controls))

    def _set_group_access_test_helper(self, group):
        '''
        Test that the files will be created with the proper data.
        '''
        all_signals = ['power', 'energy']
        all_controls = ['power', 'frequency']
        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(all_signals, all_controls)), \
             mock.patch('geopmdpy.system_files.secure_make_dirs') as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file') as mock_smf:
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            self._access_lists.set_group_access(group, ['power'], ['frequency'])

            # Were the files written in the proper order?
            test_dir = os.path.join(self._CONFIG_PATH.name,
                                    '0.DEFAULT_ACCESS' if group == '' else group)

            signal_file = os.path.join(test_dir, 'allowed_signals')
            control_file = os.path.join(test_dir, 'allowed_controls')

            mock_smd.assert_called_with(test_dir, perm_mode=0o700)
            calls = [mock.call(signal_file, 'power\n'),
                     mock.call(control_file, 'frequency\n')]
            mock_smf.assert_has_calls(calls)

        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(all_signals, all_controls)), \
             mock.patch('geopmdpy.system_files.secure_make_dirs') as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file') as mock_smf:

            self._access_lists.set_group_access_signals(group, ['power'])

            test_dir = os.path.join(self._CONFIG_PATH.name,
                                    '0.DEFAULT_ACCESS' if group == '' else group)

            signal_file = os.path.join(test_dir, 'allowed_signals')

            mock_smd.assert_called_once_with(test_dir, perm_mode=0o700)
            calls = [mock.call(signal_file, 'power\n')]
            mock_smf.assert_has_calls(calls)

        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(all_signals, all_controls)), \
             mock.patch('geopmdpy.system_files.secure_make_dirs') as mock_smd, \
             mock.patch('geopmdpy.system_files.secure_make_file') as mock_smf:

            self._access_lists.set_group_access_controls(group, ['frequency'])

            test_dir = os.path.join(self._CONFIG_PATH.name,
                                    '0.DEFAULT_ACCESS' if group == '' else group)

            control_file = os.path.join(test_dir, 'allowed_controls')

            mock_smd.assert_called_once_with(test_dir, perm_mode=0o700)
            calls = [mock.call(control_file, 'frequency\n')]
            mock_smf.assert_has_calls(calls)


    def test_set_group_access_empty(self):
        self._set_group_access_test_helper('')

    def test_set_group_access_named(self):
        class MockGrgid:
            def __init__(self):
                self.gr_gid = 1234
        ret = MockGrgid()
        with mock.patch('grp.getgrgid', return_value=ret) as mock_getgrgid:
            self._set_group_access_test_helper('1234')
        mock_getgrgid.assert_called_with(1234)

    def test_get_group_access_empty(self):
        signals, controls = self._access_lists.get_group_access('')
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_group_access_default(self):
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        signal_file = os.path.join(default_dir, 'allowed_signals')
        control_file = os.path.join(default_dir, 'allowed_controls')

        signal_lines, control_lines = \
            self._write_group_files_helper('', self._signals_expect, self._controls_expect)
        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(self._signals_expect, self._controls_expect)), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[control_lines, signal_lines]) as mock_srf:
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            signals, controls = self._access_lists.get_group_access('')
            mock_isdir.assert_called_with(default_dir)
            calls = [mock.call(control_file), mock.call(signal_file)]
            mock_srf.assert_has_calls(calls)
        self.assertEqual(set(self._signals_expect), set(signals))
        self.assertEqual(set(self._controls_expect), set(controls))

    def test_get_group_access_specified(self):
        group_dir = os.path.join(self._CONFIG_PATH.name, '1234')
        signal_file = os.path.join(group_dir, 'allowed_signals')
        control_file = os.path.join(group_dir, 'allowed_controls')

        signal_lines, control_lines = self._write_group_files_helper('1234', [], self._controls_expect)

        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(self._signals_expect, self._controls_expect)), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('grp.getgrgid', return_value=1234) as mock_getgrgid, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[control_lines, signal_lines]) as mock_srf:
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            signals, controls = self._access_lists.get_group_access('1234')
            mock_isdir.assert_called_with(group_dir)
            calls = [mock.call(control_file), mock.call(signal_file)]
            mock_srf.assert_has_calls(calls)
            mock_getgrgid.assert_called_with(1234)
        # Assert that all controls can be read
        self.assertEqual(self._controls_expect, signals)
        self.assertEqual(set(self._controls_expect), set(controls))

    def test_get_user_access_empty(self):
        signals, controls = self._access_lists.get_user_access('', os.getpid())
        self.assertEqual([], signals)
        self.assertEqual([], controls)

    def test_get_user_access_invalid(self):
        bad_user = self._bad_user_id
        err_msg = "Specified user '{}' does not exist.".format(bad_user)
        with self.assertRaisesRegex(RuntimeError, err_msg):
            self._access_lists.get_user_access(bad_user, os.getpid())

    def test_get_user_groups_invalid(self):
        bad_user = self._bad_user_id
        err_msg = r"getpwnam\(\): name not found: '{}'".format(bad_user)
        with self.assertRaisesRegex(KeyError, err_msg):
            self._access_lists._get_user_groups(bad_user)

    def test_get_user_groups_current(self):
        current_user = pwd.getpwuid(os.getuid()).pw_name
        expected_groups = [str(gid) for gid in os.getgrouplist(current_user, os.getgid())]
        groups = self._access_lists._get_user_groups(current_user)
        self.assertEqual(set(expected_groups), set(groups))

    def test_get_user_access_default(self):
        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        signal_file = os.path.join(default_dir, 'allowed_signals')
        control_file = os.path.join(default_dir, 'allowed_controls')

        signals_default = ['energy', 'frequency']
        # Test that signal access includes all controls as well
        signals_expect = ['controls','geopm', 'energy', 'frequency', 'named', 'power']
        controls_default = ['controls','geopm', 'named', 'power']
        signal_lines, control_lines = \
            self._write_group_files_helper('', signals_default, controls_default)

        with mock.patch('geopmdpy.system_files.AccessLists._get_user_groups',
                        return_value=[]), \
             mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(signals_expect, controls_default)), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('geopmdpy.system_files.secure_read_file', side_effect=[control_lines, signal_lines]) as mock_srf:
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            signals, controls = self._access_lists.get_user_access('', os.getpid())
            mock_isdir.assert_called_with(default_dir)
            calls = [mock.call(control_file), mock.call(signal_file)]
            mock_srf.assert_has_calls(calls)

        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_default), set(controls))

    def test_get_user_access_valid(self):
        named_dir = os.path.join(self._CONFIG_PATH.name, '1234')
        named_signal_file = os.path.join(named_dir, 'allowed_signals')
        named_control_file = os.path.join(named_dir, 'allowed_controls')

        default_dir = os.path.join(self._CONFIG_PATH.name, '0.DEFAULT_ACCESS')
        default_signal_file = os.path.join(default_dir, 'allowed_signals')
        default_control_file = os.path.join(default_dir, 'allowed_controls')

        signals_default = ['energy', 'frequency', 'power']
        controls_default = ['frequency', 'power']
        default_signal_lines, default_control_lines = \
            self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['not-available-anymore', 'power', 'temperature']
        controls_named = ['frequency_uncore', 'not-available-anymore-either', 'power_uncore']
        named_signal_lines, named_control_lines = \
            self._write_group_files_helper('1234', signals_named, controls_named)

        signals_avail = ['energy', 'extra_power', 'extra_temperature', 'frequency',
                         'power', 'temperature']
        controls_avail = ['extra_frequency_uncore', 'extra_power_uncore', 'frequency', 'frequency_uncore',
                          'power', 'power_uncore']

        all_signals = set(signals_named).union(set(signals_default))
        signals_expect = all_signals.intersection(signals_avail)
        all_controls = set(controls_named).union(set(controls_default))
        controls_expect = all_controls.intersection(controls_avail)

        valid_user = 'val'
        groups=['1234']
        with mock.patch('geopmdpy.system_files.AccessLists._get_user_groups',
                        return_value=groups), \
             mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(signals_avail, controls_avail)), \
             mock.patch('os.path.isdir', return_value=True) as mock_isdir, \
             mock.patch('grp.getgrgid') as mock_getgrgid, \
             mock.patch('geopmdpy.system_files.secure_read_file',
                        side_effect=[named_control_lines, named_signal_lines,
                                     default_control_lines, default_signal_lines]) as mock_srf:
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            signals, controls = self._access_lists.get_user_access(valid_user, os.getpid())
            calls = [mock.call(named_dir), mock.call(default_dir)]
            mock_isdir.assert_has_calls(calls)
            calls = [mock.call(named_control_file), mock.call(named_signal_file),
                     mock.call(default_control_file), mock.call(default_signal_file)]
            mock_srf.assert_has_calls(calls)
            mock_getgrgid.assert_called_with(1234)

        self.assertEqual(set(signals_expect), set(signals))
        self.assertEqual(set(controls_expect), set(controls))

    def test_get_user_access_root(self):
        signals_default = ['energy', 'frequency', 'power']
        controls_default = ['frequency', 'power']
        self._write_group_files_helper('', signals_default, controls_default)

        signals_named = ['power', 'temperature']
        controls_named = ['frequency_uncore', 'power_uncore']
        self._write_group_files_helper('1234', signals_named, controls_named)

        all_signals = signals_default + signals_named
        all_controls = controls_default + controls_named

        with mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(all_signals, all_controls)), \
             mock.patch('geopmdpy.system_files.has_cap_sys_admin', return_value=True):
            self._access_lists = AccessLists(self._CONFIG_PATH.name)
            signals, controls = self._access_lists.get_user_access('root', os.getpid())

        self.assertEqual(set(all_signals), set(signals))
        self.assertEqual(set(all_controls), set(controls))

    def test__read_allowed_invalid(self):
        result = self._access_lists._read_allowed('INVALID_PATH')
        self.assertEqual([], result)

    def test_existing_dir_perms_wrong(self):
        CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        os.chmod(CONFIG_PATH.name, 0o755)
        renamed_path = f'{CONFIG_PATH.name}-uuid4-INVALID'

        with mock.patch('sys.stderr.write', return_value=None) as mock_err, \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('geopmdpy.system_files._get_names', \
                        return_value=(self._signals_expect, self._controls_expect)):
            self._access_lists = AccessLists(CONFIG_PATH.name)

            calls = [
                mock.call(f'Warning: <geopm-service> {CONFIG_PATH.name} has wrong permissions, expected 0o700\n'),
                mock.call('Warning: <geopm-service> the wrong permissions were 0o755\n'),
                mock.call(f'Warning: <geopm-service> renamed invalid path {CONFIG_PATH.name} to {renamed_path}\n')
            ]
            mock_err.assert_has_calls(calls)

        group_dir_stat = os.stat(CONFIG_PATH.name)
        self.assertEqual(0o700, stat.S_IMODE(group_dir_stat.st_mode))
        CONFIG_PATH.cleanup()
        shutil.rmtree(renamed_path)

    def test_existing_dir_perms_ok(self):
        CONFIG_PATH = tempfile.TemporaryDirectory('{}_config'.format(self._test_name))
        os.chmod(CONFIG_PATH.name, 0o700)
        renamed_path = f'{CONFIG_PATH.name}-uuid4-INVALID'

        with mock.patch('sys.stderr.write', return_value=None) as mock_err, \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('geopmdpy.system_files._get_names', return_value=([],[])):
            self._access_lists = AccessLists(CONFIG_PATH.name)
            mock_err.assert_not_called()
        CONFIG_PATH.cleanup()


if __name__ == '__main__':
    unittest.main()
