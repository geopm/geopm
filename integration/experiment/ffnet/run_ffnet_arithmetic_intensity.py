#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run Arithmetic Intensity benchmark with the FFNet agent
'''

import argparse

from integration.experiment.ffnet import ffnet
from integration.experiment import machine
from integration.apps.arithmetic_intensity import arithmetic_intensity

if __name__ == '__main__':

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    ffnet.setup_run_args(parser)
    arithmetic_intensity.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = arithmetic_intensity.create_appconf(mach, args)
    ffnet.launch(app_conf=app_conf,
                 args=args,
                 experiment_cli_args=extra_args)
