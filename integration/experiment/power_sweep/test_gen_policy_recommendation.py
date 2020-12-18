#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
from gen_policy_recommendation import *



class TestPolicy(unittest.TestCase):
    @staticmethod
    def gen_artificial_df(plrange, columns, noise, samples=1):
        data = {col_name: np.concatenate(
            [columns[col_name](plrange) \
                    * np.random.uniform(1, 1+noise, len(plrange))
                for _ in range(samples)])
            for col_name in columns}
        return pandas.DataFrame(data = data,
                index = np.concatenate(samples*[plrange]))

    def test_basic(self, noise=0.0, samples=1):
        min_pl = 190
        max_pl = 280
        tdp = 280
        
        plrange = np.arange(190, 290, 10)
        
        model = {'runtime': lambda pl: 2*pl**2 - 1120 * pl + 176800,
                    'energy': lambda pl: -pl**3 + 765*pl**2 - 189e3*pl + 2e7}
        
        df = TestPolicy.gen_artificial_df(
                plrange, model, noise=noise, samples=samples)
        
        runtime_model = CubicPowerModel()
        energy_model = CubicPowerModel()
        
        runtime_model.train(df, key='runtime')
        energy_model.train(df, key='energy')
        
        assert(policy_min_energy(
                    plrange,
                    energy_model,
                    runtime_model,
                    tdp)['power'] == 210)
        assert(policy_min_energy(
                    plrange,
                    energy_model,
                    runtime_model,
                    tdp,
                    0.1)['power'] == 250)
        assert(policy_min_energy(
                    plrange,
                    energy_model,
                    runtime_model,
                    tdp,
                    0.05)['power'] == 260)

    def test_noisy(self):
        self.test_basic(noise=0.1, samples=10) 

if __name__ == '__main__':
    unittest.main()
