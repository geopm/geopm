#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example frequency sweep experiment using AMG.
'''

import argparse

from integration.experiment.frequency_sweep import frequency_sweep
from integration.apps.amg import amg

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    frequency_sweep.setup_run_args(parser)
    args, extra_cli_args = parser.parse_known_args()
    app_conf = amg.AmgAppConf(args.node_count)
    frequency_sweep.launch(app_conf=app_conf, args=args,
                           experiment_cli_args=extra_cli_args)
