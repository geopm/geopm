#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run ParRes nstream with the cpu_torch agent.
'''
import argparse

from experiment import machine
from pytorch_experiment import cpu_torch
from apps.parres import parres


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    cpu_torch.setup_run_args(parser)
    parres.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = parres.create_nstream_appconf(mach, args)
    cpu_torch.launch(app_conf=app_conf, args=args,
                     experiment_cli_args=extra_args)
