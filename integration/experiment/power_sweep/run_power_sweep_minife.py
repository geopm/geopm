#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example power sweep experiment using miniFE.
'''

import argparse

from experiment.power_sweep import power_sweep
from apps.minife import minife


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    power_sweep.setup_run_args(parser)
    args, extra_cli_args = parser.parse_known_args()
    app_conf = minife.MinifeAppConf(args.node_count)
    power_sweep.launch(app_conf=app_conf, args=args,
                       experiment_cli_args=extra_cli_args)
