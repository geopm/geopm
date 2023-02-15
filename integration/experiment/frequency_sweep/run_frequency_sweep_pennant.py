#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example frequency sweep experiment with Pennant.
'''

import argparse

from experiment.frequency_sweep import frequency_sweep
from experiment import machine
from apps.pennant import pennant

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    frequency_sweep.setup_run_args(parser)
    pennant.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = pennant.create_appconf(mach, args)
    frequency_sweep.launch(app_conf=app_conf, args=args,
                           experiment_cli_args=extra_args)
