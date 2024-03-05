#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
CPU Torch experiment running
against the DGEMM workload
'''
import argparse

from pytorch_experiment import cpu_torch
from experiment import machine
from apps.geopmbench import geopmbench

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    cpu_torch.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = geopmbench.DgemmAppConf()
    cpu_torch.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
