#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Energy savings from power capping experiment using HPCG.
'''

import argparse

from experiment import machine
from experiment.power_sweep import power_sweep
from experiment.energy_efficiency import power_balancer_energy
from apps.hpcg import hpcg


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    power_sweep.setup_run_args(parser)
    parser.set_defaults(agent_list='power_balancer')
    args, extra_cli_args = parser.parse_known_args()
    extra_cli_args.append("--geopm-record-filter=proxy_epoch,0x00000000ff4029e3,4")
    mach = machine.init_output_dir(args.output_dir)
    app_conf = hpcg.HpcgAppConf(mach)
    power_balancer_energy.launch(app_conf=app_conf, args=args,
                                 experiment_cli_args=extra_cli_args)
