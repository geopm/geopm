#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Explicit runner for the geopmdpy session-agent integration tests.

Discovers ``Test*`` modules in this directory only, so the hermetic unit
suite under ``geopmdpy/test`` is never pulled in.  ``integration_test`` is a
top-level package that sits *beside* the installed ``geopmdpy`` module (not
inside it), so run it from the ``geopm/geopmdpy`` source directory::

    cd geopm/geopmdpy
    GEOPM_RUN_GPU_INTEGRATION=1 python -m integration_test
"""
import os
import sys
import unittest

if __name__ == '__main__':
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    top_dir = os.path.join(tests_dir, '..')
    loader = unittest.TestLoader()
    tests = loader.discover(start_dir=tests_dir, pattern='Test*',
                            top_level_dir=top_dir)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(tests)
    sys.exit(not result.wasSuccessful())
