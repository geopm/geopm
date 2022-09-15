#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run MiniFE with the ffnet agent.
'''

import argparse

from ffnet_experiment import ffnet
from experiment import machine
from apps.minife import minife

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    ffnet.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = minife.MinifeAppConf(args.node_count)
    ffnet.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
