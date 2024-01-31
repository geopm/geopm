#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest

from experiment import machine
from experiment.uncore_frequency_sweep import uncore_frequency_sweep


class TestUncoreFrequencySweep(unittest.TestCase):
    def setUp(self):
        self.mach = machine.Machine()
        self.mach.signals = {'CPU_FREQUENCY_STEP': 1.0e8}

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
