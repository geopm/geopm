#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
import os
import tempfile
from unittest import mock

from geopmdpy.access import Access

class TestAccess(unittest.TestCase):
    def setUp(self):
        self._geopm_proxy = mock.MagicMock()
        self._access = Access(self._geopm_proxy)
        self._signals_expect = ['TIME',
                                'ALL_ACCESS',
                                'SIGNAL']
        self._controls_expect = ['MAX_LIMIT',
                                 'WRITABLE',
                                 'CONTROL']

    def test_signals_query(self):
        """Test user signal access list query

        Test the run() method equivalent to 'geopmaccess'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetUserAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, False, None, False, False, False, False, False)
        self._geopm_proxy.PlatformGetUserAccess.assert_called_once()
        expected_result = '\n'.join(self._signals_expect)
        self.assertEqual(expected_result, actual_result)

    def test_controls_query(self):
        """Test user control access list query

        Test the run() method equivalent to 'geopmaccess -c'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetUserAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, True, None, False, False, False, False, False)
        self._geopm_proxy.PlatformGetUserAccess.assert_called_once()
        expected_result = '\n'.join(self._controls_expect)
        self.assertEqual(expected_result, actual_result)

    def test_default_signals_query(self):
        """Test default signal access list query

        Test the run() method equivalent to 'geopmaccess -u'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, False, None, True, False, False, False, False)
        self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
        expected_result = '\n'.join(self._signals_expect)
        self.assertEqual(expected_result, actual_result)

    def test_default_controls_query(self):
        """Test default control access list query

        Test the run() method equivalent to 'geopmaccess -u -c'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, True, None, True, False, False, False, False)
        self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
        expected_result = '\n'.join(self._controls_expect)
        self.assertEqual(expected_result, actual_result)

    def test_group_signals_query(self):
        """Test group signal access list query

        Test the run() method equivalent to 'geopmaccess -g test'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, False, 'test', False, False, False, False, False)
        self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('test')
        expected_result = '\n'.join(self._signals_expect)
        self.assertEqual(expected_result, actual_result)

    def test_group_controls_query(self):
        """Test group signal access list query

        Test the run() method equivalent to 'geopmaccess -c -g test'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, True, 'test', False, False, False, False, False)
        self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('test')
        expected_result = '\n'.join(self._controls_expect)
        self.assertEqual(expected_result, actual_result)

    def test_all_signals_query(self):
        """Test all available signal query

        Test the run() method equivalent to 'geopmaccess -a'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, True, False, None, False, False, False, False, False)
        self._geopm_proxy.PlatformGetAllAccess.assert_called_once()
        expected_result = '\n'.join(self._signals_expect)
        self.assertEqual(expected_result, actual_result)

    def test_all_controls_query(self):
        """Test all available control query

        Test the run() method equivalent to 'geopmaccess -a'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, True, True, None, False, False, False, False, False)
        self._geopm_proxy.PlatformGetAllAccess.assert_called_once()
        expected_result = '\n'.join(self._controls_expect)
        self.assertEqual(expected_result, actual_result)

    def test_write_default_signals(self):
        """Test command to write to default signal access list

        Test the run() method equivalent to 'geopmaccess -w'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            self._access.run(True, False, False, None, False, False, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('',
                self._signals_expect, self._controls_expect)
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_default_controls(self):
        """Test command to write to default control access list

        Test the run() method equivalent to 'geopmaccess -w -c'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        with mock.patch('sys.stdin.readlines', return_value=self._controls_expect):
            self._access.run(True, False, True, None, False, False, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('',
                self._signals_expect, self._controls_expect)
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_invalid_signals(self):
        """Test command to write invalid default signal access list

        Test the run() method equivalent to 'geopmaccess -w'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=([], []))
        err_msg = f'Requested access to signals that are not available: {", ".join(sorted(self._signals_expect))}'
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            with self.assertRaisesRegex(RuntimeError, err_msg):
                self._access.run(True, False, False, None, False, False, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_invalid_signals_force(self):
        """Test command to write invalid default signal access list with force

        Test the run() method equivalent to 'geopmaccess -w -F'

        """
        empty_access = ([], [])
        self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=empty_access)
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            self._access.run(True, False, False, None, False, False, False, True, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('',
                self._signals_expect, [])

    def test_write_invalid_controls(self):
        """Test command to write invalid default control access list

        Test the run() method equivalent to 'geopmaccess -w -c'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=([], []))
        err_msg = f'Requested access to controls that are not available: {", ".join(sorted(self._controls_expect))}'
        with mock.patch('sys.stdin.readlines', return_value=self._controls_expect):
            with self.assertRaisesRegex(RuntimeError, err_msg):
                self._access.run(True, False, True, None, False, False, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_signals_dry_run(self):
        """Test command to dry-run write to default signal access list

        Test the run() method equivalent to 'geopmaccess -w -n'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            self._access.run(True, False, False, None, False, False, True, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_invalid_signals_dry_run(self):
        """Test command to dry-run write invalid default signal access list

        Test the run() method equivalent to 'geopmaccess -w -n'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=([], []))
        err_msg = f'Requested access to signals that are not available: {", ".join(sorted(self._signals_expect))}'
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            with self.assertRaisesRegex(RuntimeError, err_msg):
                self._access.run(True, False, False, None, False, False, True, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_group_signals(self):
        """Test command to write to group signal access list

        Test the run() method equivalent to 'geopmaccess -w -g test'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            self._access.run(True, False, False, 'test', False, False, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('test')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('test',
                self._signals_expect, self._controls_expect)
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_write_group_controls(self):
        """Test command to write to group control access list

        Test the run() method equivalent to 'geopmaccess -w -c -g test'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        with mock.patch('sys.stdin.readlines', return_value=self._controls_expect):
            self._access.run(True, False, True, 'test', False, False, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('test')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('test',
                self._signals_expect, self._controls_expect)
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

    def test_edit_signals(self):
        """Test command to edit access list

        Test the run() method equivalent to 'geopmaccess -w -e'

        """
        edit_script=f"""\
#!/bin/bash
echo >> $1
echo {self._signals_expect[0]} >> $1
"""
        suffix = 'geopm-test-access-edit-script'
        orig_editor = os.environ.get('EDITOR')
        edit_script_path = None
        try:
            with tempfile.NamedTemporaryFile(mode='w', suffix=suffix, delete=False) as fid:
                edit_script_path = fid.name
                fid.write(edit_script)
            os.chmod(edit_script_path, 0o700)
            os.environ['EDITOR'] = edit_script_path
            all_return = (self._signals_expect,
                          self._controls_expect)
            start_return = (self._signals_expect[1:],
                            self._controls_expect)
            self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
            self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=start_return)
            self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=all_return)
            self._access.run(True, False, False, None, False, False, False, False, True)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_with('')
            set_signals = self._signals_expect[1:]
            set_signals.append(self._signals_expect[0])
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('',
                set_signals, self._controls_expect)
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()
        finally:
            if orig_editor is None:
                os.environ.pop('EDITOR')
            else:
                os.environ['EDITOR'] = orig_editor
            if edit_script_path is not None:
                os.unlink(edit_script_path)

    def test_delete_default_signals(self):
        """Test command to write to default signal access list

        Test the run() method equivalent to 'geopmaccess -D'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformSetGroupAccess = mock.Mock()
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        self._geopm_proxy.PlatformGetAllAccess = mock.Mock(return_value=return_value)
        with mock.patch('sys.stdin.readlines', return_value=self._signals_expect):
            self._access.run(False, False, False, None, False, True, False, False, False)
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('',
                [], self._controls_expect)



if __name__ == '__main__':
    unittest.main()
