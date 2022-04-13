#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
import json
import geopmpy.agent


class TestAgent(unittest.TestCase):
    def test_policy_names(self):
        for agent in geopmpy.agent.names():
            policy = geopmpy.agent.policy_names(agent)
            self.assertTrue(type(policy) is list)

    def test_sample_names(self):
        for agent in geopmpy.agent.names():
            sample = geopmpy.agent.sample_names(agent)
            self.assertTrue(type(sample) is list)

    def test_agent_names(self):
        names = geopmpy.agent.names()
        expected = set(['power_balancer', 'power_governor',
                        'frequency_map', 'monitor'])
        self.assertEqual(expected, set(names))

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
