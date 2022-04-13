#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
'''Run Quantum Espresso with the monitor agent.
'''
import argparse

from experiment.monitor import monitor
from apps.qe import qe

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    monitor.setup_run_args(parser)
    qe.setup_run_args(parser)

    args, extra_args = parser.parse_known_args()
    app_conf = qe.QuantumEspressoAppConf(args.node_count, args.benchmark_name)
    monitor.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
