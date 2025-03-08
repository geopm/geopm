#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from test_helper import remove_mock_libs
from test_helper import inject_mock_libs
import unittest
from importlib import reload
import geopmpy.gffi
import geopmpy.policy_store


class TestPolicyStoreIntegration(unittest.TestCase):
    def setUp(self):
        self._have_policy_store = True
        remove_mock_libs()
        try:
            reload(geopmpy.gffi)
            reload(geopmpy.policy_store)
        except ImportError:
            self._have_policy_store = False

    def tearDown(self):
        inject_mock_libs()

    def test_all_interfaces(self):
        if not self._have_policy_store:
            self.skipTest("PolicyStore is not enabled. Skipping its tests.")
        geopmpy.policy_store.connect(':memory:')

        geopmpy.policy_store.set_best('frequency_map', 'p1', [0.5, 1])
        geopmpy.policy_store.set_default('frequency_map', [2, 4])

        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_default('invalid_agent', [])
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_default('monitor', [0.5])
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_best('invalid_agent', 'pinv', [])
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_best('monitor', 'pinv', [0.5])

        self.assertEqual([0.5, 1], geopmpy.policy_store.get_best('frequency_map', 'p1'))
        self.assertEqual([2, 4], geopmpy.policy_store.get_best('frequency_map', 'p2'))
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.get_best('power_balancer', 'p2')

        geopmpy.policy_store.disconnect()

        # Attempt accesses to a closed connection
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_best('frequency_map', 'p1', [0.5, 1])
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_default('frequency_map', [2, 4])
        with self.assertRaises(RuntimeError):
            self.assertEqual([0.5, 1], geopmpy.policy_store.get_best('frequency_map', 'p1'))


if __name__ == '__main__':
    unittest.main()
