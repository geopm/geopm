#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import json
from importlib import reload

import geopmpy.agent
from . import mock_libgeopm

MOCKED_AGENT_NAMES = ['agent1', 'agent2']
MOCKED_SAMPLE_NAMES = ['sample1', 'sample2']


def mock_agent_num_avail(num_agent):
    num_agent.__getitem__.return_value = len(MOCKED_AGENT_NAMES)
    return 0


def mock_agent_name(agent_idx, name_max, buff):
    for idx, char in enumerate(MOCKED_AGENT_NAMES[agent_idx]):
        buff[idx] = ord(char.encode())
    buff[idx] = ord(b'\x00')
    return 0


def mock_agent_num_sample(agent_name, num_sample):
    num_sample.__getitem__.return_value = len(MOCKED_SAMPLE_NAMES)
    return 0


def mock_agent_sample_name(agent_name, sample_idx, name_max, buff):
    for idx, char in enumerate(MOCKED_SAMPLE_NAMES[sample_idx]):
        buff[idx] = ord(char.encode())
    buff[idx] = ord(b'\x00')
    return 0


class TestAgent(unittest.TestCase):
    def setUp(self):
        mock_libgeopm.reset()
        mock_libgeopm.lib.geopm_agent_num_avail.side_effect = mock_agent_num_avail
        mock_libgeopm.lib.geopm_agent_name.side_effect = mock_agent_name
        mock_libgeopm.lib.geopm_agent_num_sample.side_effect = mock_agent_num_sample
        mock_libgeopm.lib.geopm_agent_sample_name.side_effect = mock_agent_sample_name

    def test_policy_names(self):
        for agent in geopmpy.agent.names():
            policy = geopmpy.agent.policy_names(agent)
            self.assertTrue(type(policy) is list)

    def test_sample_names(self):
        for agent in geopmpy.agent.names():
            sample = geopmpy.agent.sample_names(agent)
            self.assertTrue(type(sample) is list)

    def test_agent_names(self):
        agent_names = set(geopmpy.agent.names())
        expected_agent_names = {'power_balancer', 'power_governor',
                                'frequency_map', 'monitor'}
        self.assertTrue(expected_agent_names.issubset(agent_names))

    def test_json(self):
        for agent in geopmpy.agent.names():
            policy_names = geopmpy.agent.policy_names(agent)
            exp_policy = {}
            for pp in policy_names:
                exp_policy[pp] = 'NAN'
            policy_val = [float('nan')] * len(policy_names)
            json_str = geopmpy.agent.policy_json(agent, policy_val)
            res_policy = json.loads(json_str)
            self.assertEqual(exp_policy, res_policy)


if __name__ == '__main__':
    unittest.main()
