#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
        
        self.assertEqual(
                policy_min_energy(plrange,
                                  energy_model,
                                  runtime_model,
                                  tdp)['power'],
                210)
        self.assertEqual(
                policy_min_energy(plrange,
                                  energy_model,
                                  runtime_model,
                                  tdp,
                                  0.1)['power'],
                250)
        self.assertEqual(
                policy_min_energy(plrange,
                                  energy_model,
                                  runtime_model,
                                  tdp,
                                  0.05)['power'],
                260)

    def test_noisy(self):
        self.test_basic(noise=0.1, samples=20) 

if __name__ == '__main__':
    unittest.main()
