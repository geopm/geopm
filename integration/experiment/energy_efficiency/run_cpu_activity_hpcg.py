#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
CPU Activity experiment scaling core and uncore frequency
against the hpcg workload
'''
import argparse

from experiment.energy_efficiency import cpu_activity
from experiment import machine
from apps.hpcg import hpcg


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    cpu_activity.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = hpcg.HpcgAppConf(mach)
    cpu_activity.launch(app_conf=app_conf,
                   args=args,
                   experiment_cli_args=extra_args)
