#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import unittest
from unittest import mock
import tempfile

from geopmdpy.system_files import WriteLock
from geopmdpy.system_files import secure_make_dirs
from geopmdpy.system_files import GEOPM_SERVICE_RUN_PATH_PERM

with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.system_files import WriteLock

class TestWriteLock(unittest.TestCase):
    def setUp(self):
        """Create temporary directory

        """
        self._test_name = 'TestWriteLock'
        self._TEMP_DIR = tempfile.TemporaryDirectory(self._test_name)
        self._sess_path = f'{self._TEMP_DIR.name}'
        self._orig_pid = 1234
        self._other_pid = 4321

        secure_make_dirs(self._sess_path, GEOPM_SERVICE_RUN_PATH_PERM)

    def tearDown(self):
        """Clean up temporary directory

        """
        self._TEMP_DIR.cleanup()

    def test_default_creation(self):
        """Default creation of an WriteLock object

        """
        with WriteLock(self._sess_path) as write_lock:
            self.assertIsNone(write_lock.try_lock())
            out_pid = write_lock.try_lock(self._orig_pid)
            self.assertEqual(self._orig_pid, out_pid)
        with WriteLock(self._sess_path) as other_lock:
            out_pid = other_lock.try_lock(self._other_pid)
            self.assertEqual(self._orig_pid, out_pid)
            other_lock.unlock(self._orig_pid)
            out_pid = other_lock.try_lock(self._other_pid)
            self.assertEqual(self._other_pid, out_pid)
            other_lock.unlock(self._other_pid)
            self.assertIsNone(other_lock.try_lock())

    def test_nested_creation(self):
        """Nested creation of an WriteLock object

        Mimic case where concurrent DBus calls are handled by asyncio or some
        form of thread concurrency by creating a write lock within a context
        where the write lock is already held.

        """
        with WriteLock(self._sess_path) as write_lock:
            self.assertIsNone(write_lock.try_lock())
            out_pid = write_lock.try_lock(self._orig_pid)
            self.assertEqual(self._orig_pid, out_pid)
            try:
                with WriteLock(self._sess_path) as other_lock:
                    self.fail('Able to create nested WriteLock context')
            except RuntimeError as ex:
                self.assertEqual('Attempt to modify control lock file while file lock is held by the same process', str(ex))

    def test_creation_bad_path(self):
        """Create WriteLock with invalid path

        Test that if the lock file is a directory at creation time that it is
        renamed, and the created object has a functioning try_lock() and
        unlock() method.

        """
        lock_path = os.path.join(self._sess_path, 'CONTROL_LOCK')
        os.makedirs(lock_path)
        with mock.patch('sys.stderr.write') as mock_stderr, \
             mock.patch('uuid.uuid4', return_value='uuid'):
            with WriteLock(self._sess_path) as write_lock:
                self.assertIsNone(write_lock.try_lock())
                out_pid = write_lock.try_lock(self._orig_pid)
                self.assertEqual(self._orig_pid, out_pid)
            mock_stderr.assert_called_once_with(f'Warning: <geopm-service> {lock_path} is a directory, it will be renamed to {lock_path}-uuid-INVALID\n')

    def test_creation_bad_file(self):
        """Create WriteLock with invalid file

        Test that if the lock file is a file with permissions 0o644 at
        creation time that it is renamed, and the created object has a
        functioning try_lock() and unlock() method and the contents of the
        existing file to not change the behavior of the WriteLock.

        """
        lock_path = os.path.join(self._sess_path, 'CONTROL_LOCK')
        orig_mask = os.umask(0)
        with open(os.open(lock_path, os.O_CREAT | os.O_WRONLY, 0o644), 'w') as fid:
            fid.write('9999')
        os.umask(orig_mask)
        with mock.patch('sys.stderr.write') as mock_stderr, \
             mock.patch('uuid.uuid4', return_value='uuid'):
            with WriteLock(self._sess_path) as write_lock:
                self.assertIsNone(write_lock.try_lock())
                out_pid = write_lock.try_lock(self._orig_pid)
                self.assertEqual(self._orig_pid, out_pid)
            mock_stderr.assert_has_calls([mock.call(f'Warning: <geopm-service> {lock_path} was discovered with invalid permissions, it will be renamed to {lock_path}-uuid-INVALID\n'),
                                          mock.call(f'Warning: <geopm-service> the wrong permissions were 0o100644\n')])


if __name__ == '__main__':
    unittest.main()
