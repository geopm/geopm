#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import json
import math

from test_helper import mock_libgeopm
import geopmpy.agent

MOCKED_AGENT_NAMES = ['agent1', 'agent2']
MOCKED_SAMPLE_NAMES = ['sample1', 'sample2']
MOCKED_POLICY_NAMES_1 = ['param11', 'param12']
MOCKED_POLICY_NAMES_2 = ['param2']

def mock_agent_num_avail(num_agent):
    num_agent.__getitem__.return_value = len(MOCKED_AGENT_NAMES)
    return 0


def mock_agent_name(agent_idx, name_max, buff):
    name = MOCKED_AGENT_NAMES[agent_idx]
    for idx, char in enumerate(name):
        buff[idx] = ord(char.encode())
    buff[len(name)] = ord(b'\x00')
    return 0


def mock_agent_num_sample(agent_name, num_sample):
    num_sample.__getitem__.return_value = len(MOCKED_SAMPLE_NAMES)
    return 0


def mock_agent_sample_name(agent_name, sample_idx, name_max, buff):
    name = MOCKED_SAMPLE_NAMES[sample_idx]
    for idx, char in enumerate(name):
        buff[idx] = ord(char.encode())
    buff[len(name)] = ord(b'\x00')
    return 0


def mock_agent_num_policy(agent_name, num_policy):
    if agent_name.decode().find(MOCKED_AGENT_NAMES[0]) == 0:
        num_policy.__getitem__.return_value = len(MOCKED_POLICY_NAMES_1)
    elif agent_name.decode().find(MOCKED_AGENT_NAMES[1]) == 0:
        num_policy.__getitem__.return_value = len(MOCKED_POLICY_NAMES_2)
    else:
        return -1
    return 0

def mock_agent_policy_name(agent_name, policy_idx, name_max, buff):
    if agent_name.decode().find(MOCKED_AGENT_NAMES[0]) == 0:
        policy = MOCKED_POLICY_NAMES_1[policy_idx]
    elif agent_name.decode().find(MOCKED_AGENT_NAMES[1]) == 0:
        policy = MOCKED_POLICY_NAMES_2[policy_idx]
    assert(name_max > len(policy))
    for idx, char in enumerate(policy):
        buff[idx] = ord(char.encode())
    buff[len(policy)] = ord(b'\x00')
    return 0

def mock_agent_enforce_policy():
    return 0

def mock_agent_policy_json(agent_name, policy_array, policy_max, output):
    policy = dict()
    if agent_name.decode().find(MOCKED_AGENT_NAMES[0]) == 0:
        for idx, name in enumerate(MOCKED_POLICY_NAMES_1):
            policy[name] = policy_array[idx] if not math.isnan(policy_array[idx]) else 'NAN'
    elif agent_name.decode().find(MOCKED_AGENT_NAMES[1]) == 0:
        for idx, name in enumerate(MOCKED_POLICY_NAMES_2):
            policy[name] = policy_array[idx]  if not math.isnan(policy_array[idx]) else 'NAN'
    result = json.dumps(policy)
    for idx, char in enumerate(result):
        output[idx] = ord(char.encode())
    output[len(result)] = ord(b'\x00')
    return 0

class TestAgent(unittest.TestCase):
    def setUp(self):
        mock_libgeopm.reset_mock()
        mock_libgeopm.lib.geopm_agent_num_avail.side_effect = mock_agent_num_avail
        mock_libgeopm.lib.geopm_agent_name.side_effect = mock_agent_name
        mock_libgeopm.lib.geopm_agent_num_sample.side_effect = mock_agent_num_sample
        mock_libgeopm.lib.geopm_agent_sample_name.side_effect = mock_agent_sample_name
        mock_libgeopm.lib.geopm_agent_num_policy.side_effect = mock_agent_num_policy
        mock_libgeopm.lib.geopm_agent_policy_name.side_effect = mock_agent_policy_name
        mock_libgeopm.lib.geopm_agent_enforce_policy.side_effect = mock_agent_enforce_policy
        mock_libgeopm.lib.geopm_agent_policy_json = mock_agent_policy_json

    def test_policy_names(self):
        for agent in geopmpy.agent.names():
            policy = geopmpy.agent.policy_names(agent)
            self.assertTrue(type(policy) is list)

    def test_sample_names(self):
        for agent in geopmpy.agent.names():
            sample = geopmpy.agent.sample_names(agent)
            self.assertTrue(type(sample) is list)

    def test_agent_names(self):
        agent_names = geopmpy.agent.names()
        self.assertEqual(MOCKED_AGENT_NAMES, agent_names)

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
