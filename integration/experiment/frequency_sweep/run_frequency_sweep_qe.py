#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example frequency sweep experiment using Quantum Espresso.
'''

import argparse

from experiment.frequency_sweep import frequency_sweep
from apps.qe import qe

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    frequency_sweep.setup_run_args(parser)
    qe.setup_run_args(parser)

    args, extra_cli_args = parser.parse_known_args()
    app_conf = qe.QuantumEspressoAppConf(args.node_count, args.benchmark_name)
    frequency_sweep.launch(app_conf=app_conf, args=args,
                           experiment_cli_args=extra_cli_args)
