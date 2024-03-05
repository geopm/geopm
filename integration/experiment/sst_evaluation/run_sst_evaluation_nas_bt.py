#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run a power sweep with NPB BT.
'''

import argparse

from experiment.sst_evaluation import sst_evaluation
from experiment import machine
from apps.nasbt import nasbt


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    sst_evaluation.setup_run_args(parser)
    nasbt.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = nasbt.create_appconf(mach, args)
    sst_evaluation.launch(app_conf=app_conf, args=args,
                       experiment_cli_args=extra_args)
