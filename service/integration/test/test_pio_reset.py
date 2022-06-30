#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest

from geopmdpy import pio
from geopmdpy import topo


class TestPIOReset(unittest.TestCase):
    """Test the PlatformIO reset functionality

    This test exercises a use case where the PlatformIO interface
    needs to be reset to free resources, including any
    signals/controls that might've been pushed, allowing to push a
    new set of signals/conrols.
    """
    SIGNALS = ['CPUINFO::FREQ_MIN', 'CPUINFO::FREQ_MAX']

    @staticmethod
    def push_signals(signals):
        signal_idxs = []
        for s in signals:
            domain_type = pio.signal_domain_type(s)
            domain_name = topo.domain_name(domain_type)
            for dom_idx in range(topo.num_domain(domain_name)):
                sig_idx = pio.push_signal(s, domain_name, dom_idx)
                signal_idxs.append(sig_idx)
        return signal_idxs

    @staticmethod
    def print_sample(signals, signal_idxs):
        for i, idx in enumerate(signal_idxs):
            value = pio.sample(idx)
            print(f"{signals[i]}: {value}")

    def test_pio_reset(self):
        print("Pushing and reading signals...")
        signal_idxs = self.push_signals(self.SIGNALS)
        pio.read_batch()
        self.print_sample(self.SIGNALS, signal_idxs)
        with self.assertRaises(RuntimeError):
            self.push_signals(self.SIGNALS)

        print("Resetting PlatformIO...")
        pio.reset()

        print("Pushing and reading signals...")
        signal_idxs = self.push_signals(self.SIGNALS)
        pio.read_batch()
        self.print_sample(self.SIGNALS, signal_idxs)


if __name__ == '__main__':
    unittest.main()
