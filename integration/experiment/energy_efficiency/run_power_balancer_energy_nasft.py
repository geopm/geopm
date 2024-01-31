#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import argparse

from experiment import machine
from experiment.energy_efficiency import power_balancer_energy
from experiment.power_sweep import power_sweep
from apps.nasft import nasft


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    power_sweep.setup_run_args(parser)
    parser.set_defaults(agent_list='power_balancer')
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = nasft.NasftAppConf(mach)
    power_balancer_energy.launch(app_conf=app_conf, args=args,
                                 experiment_cli_args=extra_args)

