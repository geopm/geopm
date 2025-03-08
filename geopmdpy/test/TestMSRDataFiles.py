#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import os
import glob
import json
import jsonschema

@unittest.skipUnless(os.path.exists("../../docs"), "Test requires entire Git repository")
class TestMSRDataFiles(unittest.TestCase):
    def setUp(self):
        msr_schema_file = os.path.dirname(os.path.abspath(__file__)) + \
                          "/../../docs/json_schemas/msrs.schema.json"
        with open(msr_schema_file, "r") as f:
            self._MSR_SCHEMA = json.load(f)

        msr_data_dir = os.path.dirname(os.path.abspath(__file__)) + "/../../docs/json_data"
        self._MSR_DATA_FILES = glob.glob(msr_data_dir + "/msr_data_*.json")
        self.assertTrue(self._MSR_DATA_FILES)

    def test_msr_data_files(self):
        for file_name in self._MSR_DATA_FILES:
            with open(file_name, "r") as f:
                msr_data = json.load(f)
            jsonschema.validate(msr_data, schema=self._MSR_SCHEMA)


if __name__ == "__main__":
    unittest.main()
