#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import sys
import time
from unittest import mock
from geopmdpy import topo
from geopmdpy import pio
from geopmdpy import gffi


class _NoServiceLib:
    """Proxy around the real libgeopmd shared object.

    All symbols are forwarded to the real library so that the pio
    module itself is exercised unmodified.  The control save, restore,
    and write entry points are the only ones stubbed out: those are the
    calls that contact the geopm systemd service over SDBus, which
    would otherwise fail when the service is active and already holds a
    write-mode client (e.g. another running session).  Stubbing them
    here mocks the systemd interaction without mocking the pio module.

    """
    _STUBBED = frozenset((
        'geopm_pio_save_control',
        'geopm_pio_restore_control',
        'geopm_pio_write_control',
        'geopm_pio_write_batch',
    ))

    def __init__(self, real_lib):
        object.__setattr__(self, '_real_lib', real_lib)

    def __getattr__(self, name):
        if name in _NoServiceLib._STUBBED:
            return lambda *args, **kwargs: 0
        return getattr(self._real_lib, name)


class TestPIO(unittest.TestCase):
    def setUp(self):
        # Mock the systemd-backed control operations so the tests do not
        # contend with an active geopm service for the write lock.
        patcher = mock.patch.object(gffi, 'dl_geopmd',
                                    _NoServiceLib(gffi.dl_geopmd))
        patcher.start()
        self.addCleanup(patcher.stop)
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
            power = pio.read_signal('CPU_POWER', 'cpu', 0)
        except RuntimeError:
            sys.stdout.write('<warning> failed to read package power\n')

    def test_write_control(self):
        try:
            pio.write_control('CPU_FREQUENCY_MAX_CONTROL', 'package', 0, 1.0e9)
        except RuntimeError:
            sys.stdout.write('<warning> failed to write CPU frequency\n')

if __name__ == '__main__':
    unittest.main()
