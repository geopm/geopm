#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example frequency sweep experiment using Nekbone.
'''


from experiment.frequency_sweep import frequency_sweep
from apps.nekbone import nekbone


if __name__ == '__main__':

    app_conf = nekbone.NekboneAppConf()
    frequency_sweep.main(app_conf)
