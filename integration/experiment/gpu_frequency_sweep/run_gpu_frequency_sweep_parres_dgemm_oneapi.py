#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run ParRes dgemm oneAPI version with the frequency map agent.
'''

import argparse

from experiment.gpu_frequency_sweep import gpu_frequency_sweep
from experiment import machine
from apps.parres import parres

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    gpu_frequency_sweep.setup_run_args(parser)
    parres.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = parres.create_dgemm_appconf_oneapi(mach, args)
    gpu_frequency_sweep.launch(app_conf=app_conf, args=args,
                               experiment_cli_args=extra_args)
