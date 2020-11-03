#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

from experiment import machine
from experiment.uncore_frequency_sweep import uncore_frequency_sweep


class TestUncoreFrequencySweep(unittest.TestCase):
    def setUp(self):
        self.mach = machine.Machine()
        self.mach.signals = {'FREQUENCY_STEP': 1.0e8}

    def test_full_range(self):
        freqs = uncore_frequency_sweep.setup_uncore_frequency_bounds(self.mach, None, None, None)
        self.assertEqual(freqs, [2.7e9, 2.6e9, 2.5e9, 2.4e9, 2.3e9, 2.2e9, 2.1e9, 2.0e9,
                                 1.9e9, 1.8e9, 1.7e9, 1.6e9, 1.5e9, 1.4e9, 1.3e9, 1.2e9])

    def test_partial_range(self):
        freqs = uncore_frequency_sweep.setup_uncore_frequency_bounds(self.mach, 1.3e9, 1.7e9, 2e8)
        assert(freqs == [1.7e9, 1.5e9, 1.3e9])

    def test_errors(self):
        with self.assertRaises(RuntimeError):
            uncore_frequency_sweep.setup_uncore_frequency_bounds(self.mach, 0.1e9, 1.3e9, 1e8)
        with self.assertRaises(RuntimeError):
            uncore_frequency_sweep.setup_uncore_frequency_bounds(self.mach, 1.7e9, 6.0e9, 1e8)
        with self.assertRaises(RuntimeError):
            uncore_frequency_sweep.setup_uncore_frequency_bounds(self.mach, 1.7e9, 1.6e9, 1e8)


if __name__ == '__main__':
    unittest.main()
