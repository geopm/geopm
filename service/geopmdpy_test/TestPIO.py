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

from __future__ import absolute_import

import unittest
import sys
import time
from geopmdpy import topo
from geopmdpy import pio


class TestPIO(unittest.TestCase):
    def setUp(self):
        pio.save_control()

    def tearDown(self):
        pio.restore_control()

    def test_domain_name(self):
        time_domain_type = pio.signal_domain_type("TIME")
        time_domain_name = topo.domain_name(time_domain_type)
        self.assertEqual('cpu', time_domain_name)

    def test_signal_names(self):
        all_signal_names = pio.signal_names()
        self.assertEqual(list, type(all_signal_names))
        self.assertTrue('TIME' in all_signal_names)

    def test_control_names(self):
        all_control_names = pio.control_names()
        self.assertEqual(list, type(all_control_names))

    def test_read_signal(self):
        expect_t0 = time.time()
        actual_t0 = pio.read_signal('TIME', topo.DOMAIN_CPU, 0)
        time.sleep(1.0)
        expect_t1 = time.time()
        actual_t1 = pio.read_signal('TIME', topo.DOMAIN_CPU, 0)
        expect_tt = expect_t1 - expect_t0
        actual_tt = actual_t1 - actual_t0
        self.assertAlmostEqual(expect_tt, actual_tt, delta=0.1)
        try:
            power = pio.read_signal('POWER_PACKAGE', 'cpu', 0)
        except RuntimeError:
            sys.stdout.write('<warning> failed to read package power\n')

    def test_write_control(self):
        try:
            pio.write_control('FREQUENCY', 'package', 0, 1.0e9)
        except RuntimeError:
            sys.stdout.write('<warning> failed to write cpu frequency\n')

if __name__ == '__main__':
    unittest.main()
