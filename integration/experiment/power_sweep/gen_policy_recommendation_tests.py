#!/usr/bin/env python

from gen_policy_recommendation import *

def gen_artificial_df(plrange, columns, noise, samples=1):
    data = {col_name: np.concatenate(
        [columns[col_name](plrange) \
                * np.random.uniform(1, 1+noise, len(plrange))
            for _ in range(samples)])
        for col_name in columns}
    return pandas.DataFrame(data = data,
            index = np.concatenate(samples*[plrange]))

def test(noise=0.1, samples=10):
    min_pl = 190
    max_pl = 280
    tdp = 280
    
    plrange = np.arange(190, 290, 10)
    
    model = {'runtime': lambda pl: 2*pl**2 - 1120 * pl + 176800,
                'energy': lambda pl: -pl**3 + 765*pl**2 - 189e3*pl + 2e7}
    
    df = gen_artificial_df(plrange, model, noise=noise, samples=samples)
    
    runtime_model = CubicPowerModel()
    energy_model = CubicPowerModel()
    
    runtime_model.train(df, key='runtime')
    energy_model.train(df, key='energy')
    
    assert(policy_min_energy(plrange, energy_model, runtime_model, tdp, None)['power'] == 210)
    assert(policy_min_energy(plrange, energy_model, runtime_model, tdp, max_degradation=0.1)['power'] == 250)
    assert(policy_min_energy(plrange, energy_model, runtime_model, tdp, max_degradation=0.05)['power'] == 260)

test(noise=0, samples=1)
test()
