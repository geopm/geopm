#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Frequency map experiment comparing nekbone with added barriers run
at a lower frequency to the baseline with no added barriers.
'''

from experiment.energy_efficiency import barrier_frequency_sweep
from apps.nekbone import nekbone


if __name__ == '__main__':

    app_conf_ref = nekbone.NekboneAppConf(add_barriers=False)
    app_conf = nekbone.NekboneAppConf(add_barriers=True)
    barrier_frequency_sweep.main(app_conf_ref, app_conf)
