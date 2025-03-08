#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
import os
import tempfile

from geopmdpy import system_files

INITIAL_CONFIG_PATH = system_files.GEOPM_SERVICE_CONFIG_PATH
INITIAL_LEGACY_CONFIG_PATH = system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH

class TestConfigPath(unittest.TestCase):
    def setUp(self):
        self._temp_dir = tempfile.TemporaryDirectory('TestConfigPath')
        system_files.GEOPM_SERVICE_CONFIG_PATH = os.path.join(
            self._temp_dir.name, os.path.relpath(INITIAL_CONFIG_PATH, '/'))
        system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH = os.path.join(
            self._temp_dir.name, os.path.relpath(INITIAL_LEGACY_CONFIG_PATH, '/'))

        self._expected_legacy_warning = (
            'Warning <geopm-service> Using an old GEOPM configuration directory. '
            f'The directory at "{system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH}" '
            'is deprecated and may not be used in future GEOPM releases. Migrate '
            'configuration data to the directory at '
            f'"{system_files.GEOPM_SERVICE_CONFIG_PATH}"\n')

        self._expected_multi_config_warning = (
            'Warning <geopm-service> Detected multiple geopm configuration directories. '
            f'The GEOPM service is using "{system_files.GEOPM_SERVICE_CONFIG_PATH}" and '
            f'ignoring "{system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH}". The directory '
            f'at "{system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH}" is deprecated and may '
            'not be used in future GEOPM releases. Migrate configuration data to the '
            f'directory at "{system_files.GEOPM_SERVICE_CONFIG_PATH}"\n')

        system_files.get_config_path.cache_clear()

    def tearDown(self):
        self._temp_dir.cleanup()

    def test_no_existing_config_paths(self):
        with mock.patch('sys.stderr.write') as mock_sys_stderr_write:
            self.assertEqual(system_files.GEOPM_SERVICE_CONFIG_PATH, system_files.get_config_path())
            mock_sys_stderr_write.assert_not_called()

    def test_only_latest_config_path_exists(self):
        os.makedirs(system_files.GEOPM_SERVICE_CONFIG_PATH)
        with mock.patch('sys.stderr.write') as mock_sys_stderr_write:
            self.assertEqual(system_files.GEOPM_SERVICE_CONFIG_PATH, system_files.get_config_path())
            mock_sys_stderr_write.assert_not_called()

    def test_only_legacy_config_path_exists(self):
        os.makedirs(system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH)
        with mock.patch('sys.stderr.write') as mock_sys_stderr_write:
            # Call twice. Same output both times, but only warn once.
            self.assertEqual(system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH, system_files.get_config_path())
            self.assertEqual(system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH, system_files.get_config_path())
            mock_sys_stderr_write.assert_called_once_with(self._expected_legacy_warning)

    def test_multiple_config_paths_exist(self):
        os.makedirs(system_files.GEOPM_SERVICE_CONFIG_PATH)
        os.makedirs(system_files.LEGACY_GEOPM_SERVICE_CONFIG_PATH)
        with mock.patch('sys.stderr.write') as mock_sys_stderr_write:
            # Call twice. Same output both times, but only warn once.
            self.assertEqual(system_files.GEOPM_SERVICE_CONFIG_PATH, system_files.get_config_path())
            self.assertEqual(system_files.GEOPM_SERVICE_CONFIG_PATH, system_files.get_config_path())
            mock_sys_stderr_write.assert_called_with(self._expected_multi_config_warning)


if __name__ == '__main__':
    unittest.main()
