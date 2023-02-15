#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run GEOPMBENCH with the monitor agent.
'''

import argparse

from experiment.monitor import monitor
from experiment import machine
from apps.geopmbench import geopmbench


if __name__ == '__main__':

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    geopmbench.setup_geopmbench_run_args(parser)
    monitor.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = geopmbench.create_geopmbench_appconf(mach, args)
    monitor.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
