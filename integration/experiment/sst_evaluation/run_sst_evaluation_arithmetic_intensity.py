#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run an SST evaluation with the arithmetic intensity benchmark.
'''

import argparse

from experiment.sst_evaluation import sst_evaluation
from experiment import machine
from apps.arithmetic_intensity import arithmetic_intensity


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    sst_evaluation.setup_run_args(parser)
    arithmetic_intensity.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = arithmetic_intensity.create_appconf(mach, args)
    sst_evaluation.launch(app_conf=app_conf, args=args,
                          experiment_cli_args=extra_args)
