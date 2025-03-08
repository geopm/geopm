#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
from typing import Union, Callable
from geopmdpy import system_files


class RestorableFileWriter:
    def __init__(self,
                 write_path: Union[str, os.PathLike],
                 backup_path: Union[str, os.PathLike],
                 warning_handler: Union[Callable[[str], None], None] = None):
        """Create a context manager that restores the contents of backup_path
        to write_path and deletes backup_path on context exit.
        Note that only one RestorableFileWriter should use a given backup_path
        at a time since the backup file is removed as soon as any instance
        restores the backup.
        """
        self._write_path: Union[str, os.PathLike] = write_path
        self._backup_path: Union[str, os.PathLike] = backup_path
        # If no warning handler is requested, ignore warnings
        self._warning_handler: Callable[[str], None] = (
            warning_handler if warning_handler is not None else lambda warning: None)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        # This function can potentially be called while handling another
        # exception already. Log this function's exceptions so we don't clobber
        # some other exception's logged stacktrace.
        try:
            self.restore_and_cleanup()
        except Exception as e:
            self._warning_handler(f'Encountered exception in restore to {self._write_path}: {repr(e)}')

    def backup_and_try_update(self, new_file_contents: str):
        """Copy the current contents (if any) of the write_path file to the
        backup_path file, then replace the contents of the write_path file with
        new_file_contents if the write_path file exists. If write_path does not
        exist, do nothing.
        """
        try:
            with open(self._write_path, 'r') as f:
                old_setting = f.read()
        except FileNotFoundError:
            # If there is no write_path file, then there is nothing to back up
            return

        if os.path.isfile(self._backup_path):
            # The backup file already exists. This could happen if the service
            # terminated without being given a chance to restore the backup.
            # Don't overwrite the existing backup with the current write_path
            # contents, since that setting may be what the service wrote before.
            self._warning_handler(f'Reusing existing backup at {self._backup_path}')
        else:
            system_files.secure_make_file(self._backup_path, old_setting)

        with open(self._write_path, 'w') as f:
            f.write(new_file_contents)

    def restore_and_cleanup(self):
        """Restore the contents of the backup file to the target file, deleting
        the backup file after successful restore.
        """
        if os.path.exists(self._backup_path):
            backup_contents = None
            try:
                backup_contents = system_files.secure_read_file(self._backup_path)
            except Exception as e:
                self._warning_handler(f'Encountered exception in secure read of {self._backup_path}: {e}')

            if backup_contents is None:
                # If there is (or was immediately recently) backup file but
                # the secure read failed (exception or return None) then we want to know
                self._warning_handler(f'Unable to securely read {self._backup_path}')
                return
        else:
            # If there is no backup file (e.g., if there was no initial file),
            # then there is nothing to restore
            return

        with open(self._write_path, 'w') as f:
            f.write(backup_contents)
        os.remove(self._backup_path)
