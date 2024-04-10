#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example frequency sweep experiment using geopmbench.
'''


from integration.experiment.frequency_sweep import frequency_sweep
from integration.apps.geopmbench import geopmbench


if __name__ == '__main__':

    app_conf = geopmbench.TinyAppConf()
    frequency_sweep.main(app_conf, cool_off_time=0)
