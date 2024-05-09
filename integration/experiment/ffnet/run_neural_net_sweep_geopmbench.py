#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
GPU frequency sweep experiment using geopmbench.
'''

import argparse

import integration.experiment.ffnet.neural_net_sweep as neural_net_sweep
from integration.experiment import machine
from apps.geopmbench import geopmbench


if __name__ == '__main__':

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    neural_net_sweep.setup_run_args(parser)
    geopmbench.setup_geopmbench_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = geopmbench.create_geopmbench_appconf(mach, args)
    neural_net_sweep.launch(app_conf=app_conf, args=args,
                           experiment_cli_args=extra_args)
