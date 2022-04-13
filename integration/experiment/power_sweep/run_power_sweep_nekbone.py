#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run a power sweep with nekbone
'''

from experiment.power_sweep import power_sweep
from apps.nekbone import nekbone


if __name__ == '__main__':

    app_conf = nekbone.NekboneAppConf()
    power_sweep.main(app_conf)
