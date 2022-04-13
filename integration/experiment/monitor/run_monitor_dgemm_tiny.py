#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run a small DGEMM with the monitor agent.
'''


from experiment.monitor import monitor
from apps.geopmbench import geopmbench


if __name__ == '__main__':

    app_conf = geopmbench.TinyAppConf()
    monitor.main(app_conf, cool_off_time=0)
