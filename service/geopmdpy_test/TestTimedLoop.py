#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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
from time import time

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.runtime import TimedLoop

class TestTimedLoop(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestTimedLoop'

    def tearDown(self):
        pass

    def test_timed_loop_invalid(self):
        err_msg = 'Specified period is invalid.  Must be > 0.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            TimedLoop(0, 0)

        err_msg = 'Specified num_period is invalid.  Must be > 0.'
        with self.assertRaisesRegex(RuntimeError, err_msg):
            TimedLoop(1, 0)

        err_msg = 'num_period must be a whole number.'
        with self.assertRaisesRegex(ValueError, err_msg):
            TimedLoop(1.5, 1.5)

        err_msg = "'<=' not supported between instances of 'str' and 'float'"
        with self.assertRaisesRegex(TypeError, err_msg):
            TimedLoop('asd', 'dsa')

    def test_timed_loop_infinite(self):
        period = 0.01
        tl = TimedLoop(period) # Infinte loop

        self.assertEqual(period, tl._period)
        self.assertIsNone(tl._num_loop)

        t1 = None
        for index in tl:
            if t1 is not None:
                elapsed = time() - t1
                self.assertAlmostEqual(period, elapsed, delta = 0.001)
            t1 = time()
            if index == 50: # Break after 50 iterations
                break

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
