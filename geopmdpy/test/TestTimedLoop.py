#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
import os
from unittest import mock
from time import time

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.loop import TimedLoop

class TestTimedLoop(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestTimedLoop'

    def tearDown(self):
        pass

    def test_timed_loop_invalid(self):
        err_msg = 'Specified period is invalid.  Must be >= 0.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            TimedLoop(-1, 0)

        err_msg = 'Specified num_period is invalid.  Must be > 0.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            TimedLoop(1, -1)

        err_msg = 'num_period must be a whole number.'
        with self.assertRaisesRegex(ValueError, err_msg):
            TimedLoop(1.5, 1.5)

        err_msg = "'<' not supported between instances of 'str' and 'float'"
        with self.assertRaisesRegex(TypeError, err_msg):
            TimedLoop('asd', 'dsa')

    @unittest.skipIf(os.environ.get('GEOPM_TEST_EXTENDED') is None, "Requires accurate timing")
    def test_timed_loop_infinite(self):
        period = 0.01
        tl = TimedLoop(period) # Infinite loop

        self.assertEqual(period, tl._period)
        self.assertIsNone(tl._num_loop)

        t1 = None
        for index in tl:
            if t1 is not None:
                elapsed = time() - t1
                self.assertAlmostEqual(period, elapsed, delta = 0.002)
            t1 = time()
            if index == 50: # Break after 50 iterations
                break

    @unittest.skipIf(os.environ.get('GEOPM_TEST_EXTENDED') is None, "Requires accurate timing")
    def test_timed_loop_fixed(self):
        period = 0.01
        num_period = 10
        tl = TimedLoop(period, num_period)

        self.assertEqual(period, tl._period)
        self.assertEqual(num_period + 1, tl._num_loop)

        it = iter(tl)
        for ii in range(num_period + 1):
            # Start checking at 2 since 0 is really setup and the 1st iteration is not delayed
            if ii == 2:
                elapsed = time() - t1
                self.assertAlmostEqual(period, elapsed, delta = 0.001)
            t1 = time()
            self.assertEqual(ii, next(it))

        with self.assertRaisesRegex(StopIteration, ''):
            next(it)


if __name__ == '__main__':
    unittest.main()
