#!/usr/bin/env python3
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run CosmicTagger with the gpu_activity agent.
'''
import argparse

from experiment import machine
from experiment.energy_efficiency import gpu_activity
from apps.cosmictagger import cosmictagger


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    gpu_activity.setup_run_args(parser)
    args, extra_args = parser.parse_known_args()
    mach = machine.init_output_dir(args.output_dir)
    app_conf = cosmictagger.CosmicTaggerAppConf(mach, args.node_count)
    gpu_activity.launch(app_conf=app_conf, args=args,
                        experiment_cli_args=extra_args)
