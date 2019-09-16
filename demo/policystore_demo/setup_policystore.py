#!/usr/bin/env python

import sys

import geopmpy.policy_store

if __name__ == '__main__':
    policy_db_path = '/home/drguttma/policystore.db'
    if len(sys.argv) > 1:
        policy_db_path = sys.argv[1]

    NAN = float("nan")

    geopmpy.policy_store.connect(policy_db_path)

    geopmpy.policy_store.set_default("power_balancer", [152])
    geopmpy.policy_store.set_best("power_142", "power_balancer", [142, 0, 0, 0])

    geopmpy.policy_store.set_default("power_governor", [145])

    geopmpy.policy_store.set_default("energy_efficient", [NAN, 1.5e9, NAN])
    geopmpy.policy_store.set_best("scalability_hi", "energy_efficient", [NAN, NAN, 0.0])
    geopmpy.policy_store.set_best("scalability_med", "energy_efficient", [NAN, 2.1e9, 0.10])
    geopmpy.policy_store.set_best("scalability_lo", "energy_efficient", [NAN, 1.7e9, NAN])

    geopmpy.policy_store.disconnect()
