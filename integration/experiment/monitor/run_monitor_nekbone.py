#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run Nekbone with the monitor agent.
'''


from experiment.monitor import monitor
from apps.nekbone import nekbone


if __name__ == '__main__':

    app_conf = nekbone.NekboneAppConf()
    monitor.main(app_conf)
