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
import os
import stat
import tempfile
from pathlib import Path

from geopmdpy.varrun import secure_make_dirs
from geopmdpy.varrun import secure_make_file
from geopmdpy.varrun import secure_read_file
from geopmdpy.varrun import secure_remove_file

class TestSecureFiles(unittest.TestCase):
    def setUp(self):
        """Create temporary directory

        """
        self._test_name = 'TestSecureFiles'
        self._TEMP_DIR = tempfile.TemporaryDirectory(self._test_name)

    def tearDown(self):
        """Clean up temporary directory

        """
        self._TEMP_DIR.cleanup()

    def check_dir_perms(self, path):
        """Assert that the path points to a file with mode 0o700
        Assert that the user and group have the right permissions.

        """
        st = os.stat(path)
        perm_mode = stat.S_IMODE(st.st_mode)
        user_owner = st.st_uid
        group_owner = st.st_gid
        self.assertEqual(0o700, perm_mode)
        self.assertEqual(os.getuid(), user_owner)
        self.assertEqual(os.getgid(), group_owner)

    def test_pre_exists(self):
        """Usage of pre existing geopm-service directory

        Test calls secure_make_dirs() when the geopm-service
        directory has already been created.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o700)
        secure_make_dirs(sess_path)
        self.check_dir_perms(sess_path)

    def test_default_creation(self):
        """Default creation of geopm-service directory

        Test calls secure_make_dirs() when the geopm-service
        directory is not present.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        secure_make_dirs(sess_path)
        self.check_dir_perms(sess_path)

    def test_creation_link_not_dir(self):
        """The path specified is a link not a directory.

        Test calls secure_make_dirs() when the geopm-service
        provided path is not a directory, but a link. It asserts that
        a warning message is printed to standard error and that a
        new directory is created while the link is renamed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service-link'
        os.symlink('/root', sess_path)
        with mock.patch('os.path.islink', return_value=True), \
             mock.patch('os.path.isdir', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            secure_make_dirs(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} is a symbolic link, the link will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the symbolic link points to /root')
            ]
            mock_err.assert_has_calls(calls)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_file_not_dir(self):
        """The path specified is a file not a directory.

        Test calls secure_make_dirs() when the geopm-service
        provided path is not a directory, but a file. It asserts that
        a warning message is printed to standard error and that a
        new directory is created while the file is renamed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service.txt'
        filename = Path(sess_path)
        filename.touch(exist_ok=True)
        with mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=False), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            secure_make_dirs(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            msg = f'Warning: <geopm-service> {sess_path} is not a directory, it will be renamed to {renamed_path}'
            mock_err.assert_called_once_with(msg)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_bad_perms(self):
        """Directory exists with bad permissions

        Test calls secure_make_dirs() when the geopm-service
        directory is present with wrong permissions.  It asserts that
        a warning message is printed to standard error and that the
        permissions are changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o755)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid()
        bad_user.st_gid = os.getgid()
        bad_user.st_mode = 0o755
        with mock.patch('os.stat', return_value=bad_user), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            secure_make_dirs(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} has wrong permissions, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong permissions were {oct(bad_user.st_mode)}')
            ]
            mock_err.assert_has_calls(calls)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_bad_user_owner(self):
        """Directory exists with wrong user owner

        Test calls secure_make_dirs() when the geopm-service
        directory is present with wrong ownership.  It asserts that
        a warning message is printed to standard error and that the
        ownership is changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o700)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid() + 1
        bad_user.st_gid = os.getgid()
        bad_user.st_mode = 0o700
        with mock.patch('os.stat', return_value=bad_user), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            secure_make_dirs(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} has wrong user owner, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong user owner was {bad_user.st_uid}')
            ]
            mock_err.assert_has_calls(calls)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

    def test_creation_bad_group_owner(self):
        """Directory exists with wrong group owner

        Test calls secure_make_dirs() when the geopm-service
        directory is present with wrong ownership.  It asserts that
        a warning message is printed to standard error and that the
        ownership is changed.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.mkdir(sess_path, mode=0o700)
        bad_user = mock.MagicMock()
        bad_user.st_uid = os.getuid()
        bad_user.st_gid = os.getgid() + 1
        bad_user.st_mode = 0o700
        with mock.patch('os.stat', return_value=bad_user), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('uuid.uuid4', return_value='uuid4'), \
             mock.patch('sys.stderr.write', return_value=None) as mock_err:
            secure_make_dirs(sess_path)
            renamed_path = f'{sess_path}-uuid4-INVALID'
            calls = [
                mock.call(f'Warning: <geopm-service> {sess_path} has wrong group owner, it will be renamed to {renamed_path}'),
                mock.call(f'Warning: <geopm-service> the wrong group owner was {bad_user.st_gid}')
            ]
            mock_err.assert_has_calls(calls)
        self.assertTrue(os.path.exists(renamed_path))
        self.check_dir_perms(sess_path)

if __name__ == '__main__':
    unittest.main()
