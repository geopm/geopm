#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import json
from importlib import reload

import geopmpy.agent
from geopmdpy import gffi


class TestAgent(unittest.TestCase):
    def setUp(self):
        # Ensures that mocks do not leak into this test
        reload(gffi)
        reload(geopmpy.agent)

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
        if geopmpy.version.__beta__:
            expected_agent_names.add('cpu_activity')
            expected_agent_names.add('gpu_activity')
            expected_agent_names.add('ffnet')
        self.assertEqual(expected_agent_names, agent_names)

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
