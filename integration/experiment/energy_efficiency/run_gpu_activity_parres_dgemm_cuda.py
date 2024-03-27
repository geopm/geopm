#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run ParRes dgemm with the gpu_activity agent.
'''
import argparse

from experiment import machine
from experiment.energy_efficiency import gpu_activity
from apps.parres import parres


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    gpu_activity.setup_run_args(parser)
    parres.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = parres.create_dgemm_appconf_cuda(mach, args)
    gpu_activity.launch(app_conf=app_conf, args=args,
                        experiment_cli_args=extra_args)
