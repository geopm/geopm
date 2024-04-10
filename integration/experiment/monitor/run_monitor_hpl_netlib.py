#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run HPL Netlib with the monitor agent.
'''

import argparse

from integration.experiment.monitor import monitor
from integration.experiment import machine
from integration.apps.hpl_netlib import hpl_netlib


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    monitor.setup_run_args(parser)
    hpl_netlib.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = hpl_netlib.HplNetlibAppConf(num_nodes=args.node_count,
                                           mach=mach,
                                           frac_dram_per_node=args.frac_dram_per_node)
    monitor.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
