#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example power sweep experiment using miniFE.
'''

import argparse

from integration.experiment.power_sweep import power_sweep
from integration.apps.minife import minife
from integration.experiment import machine


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    power_sweep.setup_run_args(parser)
    args, extra_cli_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = minife.create_appconf(mach, args)
    power_sweep.launch(app_conf=app_conf, args=args,
                       experiment_cli_args=extra_cli_args)
