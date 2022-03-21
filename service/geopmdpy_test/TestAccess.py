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

    def test_default_signals_query(self):
        """Test default signal access list query

        Test the run() method equivalent to 'geopmaccess'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, False, '')
        self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
        expected_result = '\n'.join(self._signals_expect)
        self.assertEqual(expected_result, actual_result)

    def test_default_controls_query(self):
        """Test default control access list query

        Test the run() method equivalent to 'geopmaccess -c'

        """
        return_value = (self._signals_expect,
                        self._controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, True, '')
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
        actual_result = self._access.run(False, False, False, 'test')
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
        actual_result = self._access.run(False, False, True, 'test')
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
        actual_result = self._access.run(False, True, False, '')
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
        actual_result = self._access.run(False, True, True, '')
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
            self._access.run(True, False, False, '')
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
            self._access.run(True, False, True, '')
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
                self._access.run(True, False, False, '')
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('')
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

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
                self._access.run(True, False, True, '')
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
            self._access.run(True, False, False, 'test')
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
            self._access.run(True, False, True, 'test')
            self._geopm_proxy.PlatformGetGroupAccess.assert_called_once_with('test')
            self._geopm_proxy.PlatformSetGroupAccess.assert_called_once_with('test',
                self._signals_expect, self._controls_expect)
            self._geopm_proxy.PlatformGetAllAccess.assert_called_once()

if __name__ == '__main__':
    unittest.main()
