#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run Intel (R) MKL HPL with the monitor agent.
'''

import argparse

from experiment.monitor import monitor
from experiment import machine
from apps.hpl_mkl import hpl_mkl
from apps.hpl_netlib import hpl_netlib

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    monitor.setup_run_args(parser)
    hpl_netlib.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = hpl_mkl.HplMklAppConf(num_nodes=args.node_count,
                                     mach=mach,
                                     frac_dram_per_node=args.frac_dram_per_node)
    monitor.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
