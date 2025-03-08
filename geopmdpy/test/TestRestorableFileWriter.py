#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from geopmdpy.restorable_file_writer import RestorableFileWriter
from geopmdpy import system_files
import os
import stat
import tempfile
import unittest


class TestRestorableFileWriter(unittest.TestCase):
    def setUp(self):
        self._temp_dir = tempfile.TemporaryDirectory('TestRestorableFileWriter')
        self._write_path = os.path.join(self._temp_dir.name, f'{self.id()}.write')
        self._backup_path = os.path.join(self._temp_dir.name, f'{self.id()}.backup')

    def tearDown(self):
        self._temp_dir.cleanup()

    def test_modify_existing_file(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: write_path file is populated
        with open(self._write_path, 'w') as f:
            f.write('old contents')

        writer.backup_and_try_update('Updated text')

        self.assertCountEqual([], warnings)
        with open(self._backup_path) as f:
            self.assertEqual('old contents', f.read())
        with open(self._write_path) as f:
            self.assertEqual('Updated text', f.read())

    def test_modify_is_no_op_if_no_initial_file(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: write_path file does not exist
        writer.backup_and_try_update('Updated text')

        self.assertCountEqual([], warnings)
        self.assertFalse(os.path.exists(self._write_path))
        self.assertFalse(os.path.exists(self._backup_path))

    def test_modify_does_not_overwrite_existing_backup(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: a backup already exists
        with open(self._write_path, 'w') as f:
            f.write('write_path contents')
        system_files.secure_make_file(self._backup_path, 'backed-up contents')

        writer.backup_and_try_update('Updated text')

        self.assertEqual(1, len(warnings))
        self.assertIn('Reusing existing backup', warnings[0])
        with open(self._backup_path) as f:
            self.assertEqual('backed-up contents', f.read())
        with open(self._write_path) as f:
            self.assertEqual('Updated text', f.read())

    def test_restore_saved_file(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: write_path file is backed up and overwritten
        with open(self._write_path, 'w') as f:
            f.write('overwritten contents')
        system_files.secure_make_file(self._backup_path, 'backed-up contents')

        writer.restore_and_cleanup()

        self.assertCountEqual([], warnings)
        self.assertFalse(os.path.exists(self._backup_path))
        with open(self._write_path) as f:
            self.assertEqual('backed-up contents', f.read())

    def test_restore_is_no_op_if_no_backup_file(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: backup_path file does not exist
        with open(self._write_path, 'w') as f:
            f.write('write_path contents')

        writer.restore_and_cleanup()

        self.assertCountEqual([], warnings)
        with open(self._write_path) as f:
            self.assertEqual('write_path contents', f.read())
        self.assertFalse(os.path.exists(self._backup_path))

    def test_context_manager_restores_backup(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: write_path file is populated
        with open(self._write_path, 'w') as f:
            f.write('old contents')

        with writer:
            writer.backup_and_try_update('Updated text')

            # Overwritten while in context
            with open(self._write_path) as f:
                self.assertEqual('Updated text', f.read())

        self.assertCountEqual([], warnings)

        # Restored after exiting context
        with open(self._write_path) as f:
            self.assertEqual('old contents', f.read())

    def test_context_manager_warns_on_cleanup_failure(self):
        warnings = list()
        writer = RestorableFileWriter(
            write_path=self._write_path,
            backup_path=self._backup_path,
            warning_handler=warnings.append)

        # Test-case precondition: write_path file is populated
        with open(self._write_path, 'w') as f:
            f.write('old contents')

        with writer:
            writer.backup_and_try_update('Updated text')
            # Inject a failure on restore by making the to-be-updated file read-only
            os.chmod(self._write_path, stat.S_IREAD | stat.S_IRGRP | stat.S_IROTH)

        self.assertEqual(1, len(warnings))
        self.assertIn('Encountered exception in restore', warnings[0])

if __name__ == '__main__':
    unittest.main()
