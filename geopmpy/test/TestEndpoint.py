#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
# Aliasing "mock_libgeopm" to keep the diff small when adding this. Alternatively,
# replace usage in this file with "mock_libgeopm.lib"
from test_helper import mock_libgeopm as _mock_libgeopm
mock_libgeopm = _mock_libgeopm.lib
import geopmpy.endpoint

class TestEndpoint(unittest.TestCase):
    def setUp(self):
        mock_libgeopm.reset()
        mock_libgeopm.geopm_endpoint_create.return_value = 0
        mock_libgeopm.geopm_endpoint_destroy.return_value = 0
        mock_libgeopm.geopm_endpoint_open.return_value = 0
        mock_libgeopm.geopm_endpoint_close.return_value = 0

        self.test_agent_name = 'my_agent'
        def mock_agent(endpoint, name_max, name_cstr):
            for idx, char in enumerate(self.test_agent_name):
                name_cstr[idx] = ord(char.encode())
            name_cstr[len(self.test_agent_name)] = ord(b'\x00')
            return 0
        mock_libgeopm.geopm_endpoint_agent.side_effect = mock_agent

        self._endpoint = geopmpy.endpoint.Endpoint('test_endpoint')

    def test_endpoint_creation_destruction(self):
        self.assertEqual("Endpoint(name='test_endpoint')", repr(self._endpoint))

        initial_destroy_count = mock_libgeopm.geopm_endpoint_destroy.call_count
        del self._endpoint
        self.assertEqual(initial_destroy_count + 1, mock_libgeopm.geopm_endpoint_destroy.call_count)

        mock_libgeopm.geopm_endpoint_create.return_value = 1
        self.assertRaises(RuntimeError, geopmpy.endpoint.Endpoint, 'test_endpoint')

    def test_endpoint_entry_exit(self):
        initial_open_count = mock_libgeopm.geopm_endpoint_open.call_count
        initial_close_count = mock_libgeopm.geopm_endpoint_close.call_count
        with self._endpoint:
            self.assertEqual(initial_open_count + 1, mock_libgeopm.geopm_endpoint_open.call_count)
            self.assertEqual(initial_close_count, mock_libgeopm.geopm_endpoint_close.call_count)
        self.assertEqual(initial_close_count + 1, mock_libgeopm.geopm_endpoint_close.call_count)

    def test_endpoint_agent_name(self):
        self.assertEqual(self.test_agent_name, self._endpoint.agent())

    def test_wait_for_agent_attach(self):
        mock_libgeopm.geopm_endpoint_wait_for_agent_attach.return_value = 1
        self.assertRaises(RuntimeError, self._endpoint.wait_for_agent_attach, 123.4)

        mock_libgeopm.geopm_endpoint_wait_for_agent_attach.return_value = 0
        self._endpoint.wait_for_agent_attach(123.4)

    def test_stop_wait_loop(self):
        mock_libgeopm.geopm_endpoint_wait_for_agent_stop_wait_loop.return_value = 1
        self.assertRaises(RuntimeError, self._endpoint.stop_wait_loop)

        mock_libgeopm.geopm_endpoint_wait_for_agent_stop_wait_loop.return_value = 0
        self._endpoint.stop_wait_loop()

    def test_reset_wait_loop(self):
        mock_libgeopm.geopm_endpoint_wait_for_agent_reset_wait_loop.return_value = 1
        self.assertRaises(RuntimeError, self._endpoint.reset_wait_loop)

        mock_libgeopm.geopm_endpoint_wait_for_agent_reset_wait_loop.return_value = 0
        self._endpoint.reset_wait_loop()

    def test_endpoint_profile_name(self):
        test_profile_name = 'my profile'

        def mock_profile_name(endpoint, name_max, name_cstr):
            for idx, char in enumerate(test_profile_name):
                name_cstr[idx] = ord(char.encode())
            name_cstr[len(test_profile_name)] = ord(b'\x00')
            return 0
        mock_libgeopm.geopm_endpoint_profile_name.side_effect = mock_profile_name
        self.assertEqual(test_profile_name, self._endpoint.profile_name())

    def test_endpoint_nodes(self):
        test_node_names = ['node 1', 'node 2']

        def mock_num_node(endpoint, num_node_p):
            num_node_p.__getitem__.return_value = len(test_node_names)
            return 0
        mock_libgeopm.geopm_endpoint_num_node.side_effect = mock_num_node

        def mock_node_name(endpoint, node_idx, name_max, name_cstr):
            for idx, char in enumerate(test_node_names[node_idx]):
                name_cstr[idx] = ord(char.encode())
            name_cstr[len(test_node_names[node_idx])] = ord(b'\x00')
            return 0
        mock_libgeopm.geopm_endpoint_node_name.side_effect = mock_node_name

        self.assertEqual(test_node_names, self._endpoint.nodes())

    def test_write_policy(self):
        test_policy = {'p0': 0, 'p1': 1}
        mock_libgeopm.geopm_endpoint_write_policy.return_value = 0
        with mock.patch('geopmpy.agent.policy_names') as policy_mock:
            policy_mock.return_value = list(test_policy)
            self._endpoint.write_policy(test_policy)
        args = mock_libgeopm.geopm_endpoint_write_policy.call_args[0]
        _, num_policy, policy_array = args
        self.assertEqual(num_policy, len(test_policy))
        self.assertEqual(policy_array[0], 0)
        self.assertEqual(policy_array[1], 1)

    def test_read_sample(self):
        test_sample = {'s0': 0, 's1': 1}
        test_age = 1.1

        def mock_read_sample(endpoint, num_sample, sample_array, sample_age_p):
            sample_array[0] = test_sample['s0']
            sample_array[1] = test_sample['s1']
            sample_age_p.__getitem__.return_value = test_age
            return 0
        mock_libgeopm.geopm_endpoint_read_sample.side_effect = mock_read_sample

        with mock.patch('geopmpy.agent.sample_names') as sample_mock:
            sample_mock.return_value = list(test_sample)
            self.assertEqual((test_age, test_sample),
                             self._endpoint.read_sample())


if __name__ == '__main__':
    unittest.main()
