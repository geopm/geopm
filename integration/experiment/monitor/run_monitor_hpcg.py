#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run HPCG with the monitor agent.
'''

import argparse

from experiment import machine
from experiment.monitor import monitor
from apps.hpcg import hpcg


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    monitor.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = hpcg.HpcgAppConf(mach)
    monitor.launch(app_conf=app_conf,
                   args=args,
                   experiment_cli_args=extra_args)
