#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Feed Forward net experiment using geopmbench.
'''

import argparse

from experiment.ffnet import ffnet
from experiment import machine
from apps.geopmbench import geopmbench


if __name__ == '__main__':

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    ffnet.setup_run_args(parser)
    geopmbench.setup_geopmbench_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = geopmbench.create_geopmbench_appconf(mach, args)
    ffnet.launch(app_conf=app_conf, args=args,
                           experiment_cli_args=extra_args)
