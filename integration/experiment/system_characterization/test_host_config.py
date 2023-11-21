#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
import host_config
import os
import sys
import shutil

class TestCombine(unittest.TestCase):
    """Test the host_config module combine method
    """
    def setUp(self):
        """Create an example directory structure
        """
        test_name = 'test_host_config'
        self.test_dir = test_name
        self.global_config_path = f'{test_name}_global.json'
        self.readme_path = f'{test_name}/README'
        mcfly1_config_path = f'{test_name}/mcfly1.json'
        mcfly2_config_path = f'{test_name}/mcfly2.json'
        mcfly3_config_path = f'{test_name}/mcfly3.json'
        self.all_paths = [self.global_config_path,
                          mcfly1_config_path,
                          mcfly2_config_path,
                          mcfly3_config_path]
        single_config = """
{
    "GPU_CORE_FREQUENCY_MAX": {
        "domain": "gpu",
        "description": "Defines the max core frequency to use for available GPUs",
        "units": "hertz",
        "aggregation": "average",
        "values": [1200, 1300, 1500]
    }
}
"""
        self.expected = """\
{
    "GPU_CORE_FREQUENCY_MAX": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    },
    "GPU_CORE_FREQUENCY_MAX@mcfly1": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    },
    "GPU_CORE_FREQUENCY_MAX@mcfly2": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    },
    "GPU_CORE_FREQUENCY_MAX@mcfly3": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    }
}\
"""
        self.expected_no_global = """\
{
    "GPU_CORE_FREQUENCY_MAX@mcfly1": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    },
    "GPU_CORE_FREQUENCY_MAX@mcfly2": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    },
    "GPU_CORE_FREQUENCY_MAX@mcfly3": {
        "aggregation": "average",
        "description": "Defines the max core frequency to use for available GPUs",
        "domain": "gpu",
        "units": "hertz",
        "values": [
            1200,
            1300,
            1500
        ]
    }
}\
"""
        self.readme = """
These files are populated for testing the host_config.py
"""
        os.mkdir(self.test_dir)
        for path in self.all_paths:
            with open(path, 'w') as fid:
                fid.write(single_config)
        with open(self.readme_path, 'w') as fid:
            fid.write(self.readme)

    def tearDown(self):
        """Delete the example directory structure
        """
        os.unlink(self.readme_path)
        for path in self.all_paths:
            os.unlink(path)
        os.rmdir(self.test_dir)

    def test_combine(self):
        """Check that the combined output is as expected
        """
        result = host_config.combine(self.test_dir, self.global_config_path)
        self.assertEqual(self.expected, result)

    def test_combine_no_global(self):
        """Check that the combined output is as expected without global
        """
        result = host_config.combine(self.test_dir, None)
        self.assertEqual(self.expected_no_global, result)

    def test_combine_bad_json(self):
        """Check that invalid JSON results in exception
        """
        bad_path = f'{self.readme_path}.json'
        shutil.copy(self.readme_path, bad_path)
        self.all_paths.append(bad_path)
        with open(bad_path, 'w') as fid:
            fid.write(self.readme)
        with self.assertRaises(RuntimeError):
            host_config.combine(self.test_dir, self.global_config_path)

    def test_combine_bad_append(self):
        """Check that appending an invalid JSON results in exception
        """
        shutil.copy(self.readme_path, self.global_config_path)
        with self.assertRaises(RuntimeError):
            host_config.combine(self.test_dir, self.global_config_path)

if __name__ == '__main__':
    unittest.main()
