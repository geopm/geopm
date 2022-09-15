#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run Nekbone with the monitor agent.
'''
import argparse

from ffnet_experiment import ffnet
from experiment import machine
from apps.nekbone import nekbone


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    ffnet.setup_run_args(parser)
    app_conf = nekbone.NekboneAppConf()
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    ffnet.launch(app_conf=app_conf, args=args,
                 experiment_cli_args=extra_args)
    monitor.main(app_conf)
