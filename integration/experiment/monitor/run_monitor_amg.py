#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run AMG with the monitor agent.
'''

import argparse

from integration.experiment.monitor import monitor
from integration.apps.amg import amg


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    monitor.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    app_conf = amg.AmgAppConf(args.node_count)
    monitor.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
