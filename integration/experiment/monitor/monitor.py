#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running the monitor agent.
'''

import argparse
import os

from experiment import launch_util
from experiment import common_args

def setup_run_args(parser):
    common_args.setup_run_args(parser)


def report_signals():
    return ["CPU_CYCLES_THREAD@package", "CPU_CYCLES_REFERENCE@package",
            "TIME@package", "CPU_ENERGY@package"]


def trace_signals():
    return ['MSR::UNCORE_PERF_STATUS:FREQ@package', 'MSR::UNCORE_RATIO_LIMIT:MAX_RATIO@package',
            'MSR::UNCORE_RATIO_LIMIT:MIN_RATIO@package']


def launch(app_conf, args, experiment_cli_args):
    output_dir = os.path.abspath(args.output_dir)
    extra_cli_args = launch_util.geopm_signal_args(report_signals(), trace_signals())
    extra_cli_args += experiment_cli_args

    targets = [launch_util.LaunchConfig(app_conf=app_conf,
                                        agent_conf=None,
                                        name="")]

    launch_util.launch_all_runs(targets=targets,
                                num_nodes=args.node_count,
                                iterations=args.trial_count,
                                extra_cli_args=extra_cli_args,
                                output_dir=output_dir,
                                cool_off_time=args.cool_off_time,
                                enable_traces=args.enable_traces,
                                enable_profile_traces=args.enable_profile_traces,
                                init_control_path=args.init_control)

def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_args)
