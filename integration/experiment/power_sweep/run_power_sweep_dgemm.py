#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example power sweep experiment using geopmbench.
'''


from integration.experiment.power_sweep import power_sweep
from integration.apps.geopmbench import geopmbench


if __name__ == '__main__':

    app_conf = geopmbench.DgemmAppConf()
    power_sweep.main(app_conf)
