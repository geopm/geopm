#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
CPU Activity experiment scaling core and uncore frequency
against the AMG workload
'''
import argparse

from experiment.energy_efficiency import cpu_activity
from apps.amg import amg


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    cpu_activity.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    app_conf = amg.AmgAppConf(args.node_count)
    cpu_activity.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
