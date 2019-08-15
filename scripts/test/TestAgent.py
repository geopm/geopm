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

from __future__ import absolute_import

from builtins import str
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
        expected = set(['energy_efficient', 'power_balancer', 'power_governor',
                        'frequency_map', 'monitor'])
        self.assertEqual(expected, set(names))

    def test_json(self):
        for agent in geopmpy.agent.names():
            policy_names = geopmpy.agent.policy_names(agent)
            exp_policy = {}
            for pp in policy_names:
                exp_policy[str(pp)] = u'NAN'
            policy_val = [float('nan')] * len(policy_names)
            json_str = geopmpy.agent.policy_json(agent, policy_val)
            res_policy = json.loads(json_str)
            self.assertEqual(exp_policy, res_policy)


if __name__ == '__main__':
    unittest.main()
