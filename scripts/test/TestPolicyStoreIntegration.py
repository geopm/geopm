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

import unittest
import geopmpy.policy_store
import geopmpy.version

class TestPolicyStoreIntegration(unittest.TestCase):
    @unittest.skipIf(geopmpy.version.__beta__ == False, "PolicyStoreIntegration requires beta features")
    def test_all_interfaces(self):
        geopmpy.policy_store.connect(':memory:')

        geopmpy.policy_store.set_best('p1', 'frequency_map', [0.5, 1])
        geopmpy.policy_store.set_default('frequency_map', [2, 4])
        self.assertEqual([0.5, 1], geopmpy.policy_store.get_best('p1', 'frequency_map'))
        self.assertEqual([2, 4], geopmpy.policy_store.get_best('p2', 'frequency_map'))
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.get_best('p2', 'power_balancer')

        geopmpy.policy_store.disconnect()

        # Attempt accesses to a closed connection
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_best('p1', 'frequency_map', [0.5, 1])
        with self.assertRaises(RuntimeError):
            geopmpy.policy_store.set_default('frequency_map', [2, 4])
        with self.assertRaises(RuntimeError):
            self.assertEqual([0.5, 1], geopmpy.policy_store.get_best('p1', 'frequency_map'))

if __name__ == '__main__':
    unittest.main()
