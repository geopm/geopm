#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Frequency sweep experiment using HPCG reference version.
'''

import argparse

from experiment.frequency_sweep import frequency_sweep
from experiment import machine
from apps.hpcg import hpcg

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    frequency_sweep.setup_run_args(parser)
    args, extra_cli_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = hpcg.HpcgAppConf(mach)
    frequency_sweep.launch(app_conf=app_conf, args=args,
                           experiment_cli_args=extra_cli_args)
