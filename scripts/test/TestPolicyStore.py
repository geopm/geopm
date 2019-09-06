#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
import mock
import geopm_context
try:
    from importlib import reload
except ImportError:
    # reload is built-in in python 2, and is part of importlib in Python 3.4+
    pass

mock_c = mock.MagicMock()
import geopmpy.policy_store

class TestPolicyStore(unittest.TestCase):
    def setUp(self):
        mock_c.reset_mock()
        with mock.patch('cffi.FFI.dlopen', return_value=mock_c):
            reload(geopmpy.policy_store)

    def tearDown(self):
        # Reset the mocked interface for other tests
        reload(geopmpy.policy_store)

    def test_connect(self):
        mock_c.geopm_policystore_connect.return_value = 0
        geopmpy.policy_store.connect('qwerty')
        # Expect connect(path)
        self.assertEqual(['q', 'w', 'e', 'r', 't', 'y', '\0'],
                         list(mock_c.geopm_policystore_connect.call_args[0][0]))

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
        geopmpy.policy_store.get_best('p1', 'a1')
        # Expect get_best(profile, agent)
        self.assertEqual(['p', '1', '\0'], list(mock_c.geopm_policystore_get_best.call_args[0][0]))
        self.assertEqual(['a', '1', '\0'], list(mock_c.geopm_policystore_get_best.call_args[0][1]))

        mock_c.geopm_policystore_get_best.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.get_best('p1', 'a1')

    def test_set_best(self):
        mock_c.geopm_policystore_set_best.return_value = 0
        geopmpy.policy_store.set_best('p1', 'a1', [1., 2.])
        # Expect set_best(profile, agent, policy_values, default_policy)
        self.assertEqual(['p', '1', '\0'], list(mock_c.geopm_policystore_set_best.call_args[0][0]))
        self.assertEqual(['a', '1', '\0'], list(mock_c.geopm_policystore_set_best.call_args[0][1]))
        self.assertEqual(2, mock_c.geopm_policystore_set_best.call_args[0][2])
        self.assertEqual([1, 2], list(mock_c.geopm_policystore_set_best.call_args[0][3]))

        mock_c.geopm_policystore_set_best.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_best('p1', 'a1', [1., 2.])

    def test_set_default(self):
        mock_c.geopm_policystore_set_default.return_value = 0
        geopmpy.policy_store.set_default('a1', [1., 2.])
        # Expect set_default(agent, policy_values, default_policy)
        self.assertEqual(['a', '1', '\0'], list(mock_c.geopm_policystore_set_default.call_args[0][0]))
        self.assertEqual(2, mock_c.geopm_policystore_set_default.call_args[0][1])
        self.assertEqual([1, 2], list(mock_c.geopm_policystore_set_default.call_args[0][2]))

        mock_c.geopm_policystore_set_default.return_value = -1
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_default('a1', [1., 2.])

if __name__ == '__main__':
    unittest.main()
