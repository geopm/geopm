#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import geopmpy.policy_store
import geopmpy.version


class TestPolicyStoreIntegration(unittest.TestCase):
    @unittest.skipIf(not geopmpy.version.__beta__, "PolicyStoreIntegration requires beta features")
    def test_all_interfaces(self):
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
