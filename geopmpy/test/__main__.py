#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import unittest
import os
import sys

if __name__ == "__main__":
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    top_dir = os.path.join(tests_dir, '..')
    loader = unittest.TestLoader()
    tests = loader.discover(start_dir=tests_dir, pattern='Test*', top_level_dir=top_dir)
    runner = unittest.TextTestRunner()
    result = runner.run(tests)
    sys.exit(not result.wasSuccessful())
