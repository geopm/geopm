#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
from unittest import mock
from importlib import reload

mock_c = mock.MagicMock()
import geopmpy.policy_store
import geopmdpy.gffi

class TestPolicyStore(unittest.TestCase):
    def setUp(self):
        mock_c.reset_mock()
        with mock.patch('cffi.FFI.dlopen', return_value=mock_c):
            reload(geopmdpy.gffi)
            reload(geopmpy.policy_store)

    def tearDown(self):
        # Reset the mocked interface for other tests
        reload(geopmdpy.gffi)
        reload(geopmpy.policy_store)

    def test_connect(self):
        mock_c.geopm_policystore_connect.return_value = 0
        geopmpy.policy_store.connect('qwerty')
        # Expect connect(path)
        self.assertEqual(b'qwerty\0',
                         b''.join(mock_c.geopm_policystore_connect.call_args[0][0]))

        # Expect a raised exception when there is a non-zero return code
        mock_c.geopm_policystore_connect.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.connect('qwerty')

    def test_disconnect(self):
        mock_c.geopm_policystore_disconnect.return_value = 0
        geopmpy.policy_store.disconnect()

        mock_c.geopm_policystore_disconnect.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.disconnect()

    def test_get_best(self):
        mock_c.geopm_policystore_get_best.return_value = 0
        geopmpy.policy_store.get_best('a1', 'p1')
        # Expect get_best(profile, agent)
        self.assertEqual(b'a1\0', b''.join(mock_c.geopm_policystore_get_best.call_args[0][0]))
        self.assertEqual(b'p1\0', b''.join(mock_c.geopm_policystore_get_best.call_args[0][1]))

        mock_c.geopm_policystore_get_best.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.get_best('a1', 'p1')

    def test_set_best(self):
        mock_c.geopm_policystore_set_best.return_value = 0
        geopmpy.policy_store.set_best('a1', 'p1', [1., 2.])
        # Expect set_best(profile, agent, policy_values, default_policy)
        self.assertEqual(b'a1\0', b''.join(mock_c.geopm_policystore_set_best.call_args[0][0]))
        self.assertEqual(b'p1\0', b''.join(mock_c.geopm_policystore_set_best.call_args[0][1]))
        self.assertEqual(2, mock_c.geopm_policystore_set_best.call_args[0][2])
        self.assertEqual([1, 2], list(mock_c.geopm_policystore_set_best.call_args[0][3]))

        mock_c.geopm_policystore_set_best.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_best('a1', 'p1', [1., 2.])

    def test_set_default(self):
        mock_c.geopm_policystore_set_default.return_value = 0
        geopmpy.policy_store.set_default('a1', [1., 2.])
        # Expect set_default(agent, policy_values, default_policy)
        self.assertEqual(b'a1\0', b''.join(mock_c.geopm_policystore_set_default.call_args[0][0]))
        self.assertEqual(2, mock_c.geopm_policystore_set_default.call_args[0][1])
        self.assertEqual([1, 2], list(mock_c.geopm_policystore_set_default.call_args[0][2]))

        mock_c.geopm_policystore_set_default.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_default('a1', [1., 2.])


if __name__ == '__main__':
    unittest.main()
