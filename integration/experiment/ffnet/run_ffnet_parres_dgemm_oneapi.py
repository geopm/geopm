#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run ParRes dgemm oneAPI version with the frequency map agent.
'''

import argparse

from integration.experiment.ffnet import ffnet
from integration.experiment import machine
from integration.apps.parres import parres

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    ffnet.setup_run_args(parser)
    parres.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = parres.create_dgemm_appconf_oneapi(mach, args)
    ffnet.launch(app_conf=app_conf, args=args,
                               experiment_cli_args=extra_args)
