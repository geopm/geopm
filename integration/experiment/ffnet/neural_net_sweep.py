#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helper functions for running gpu frequency sweep experiments.
'''

import sys
import argparse
import os

import geopmpy.agent

from experiment import launch_util
from experiment import common_args
from experiment import machine
from experiment.frequency_sweep import frequency_sweep
from experiment.uncore_frequency_sweep import uncore_frequency_sweep
import experiment.gpu_sweep


def report_signals():
    cpu_signals = ["CPU_CYCLES_THREAD@package", "CPU_CYCLES_REFERENCE@package",
            "TIME@package", "CPU_ENERGY@package"]
    gpu_signals = ["GPU_CORE_FREQUENCY_STATUS@board",
            "GPU_CORE_FREQUENCY_MIN_CONTROL@board", "GPU_CORE_FREQUENCY_MAX_CONTROL@board"]

    if machine.num_gpu() > 0:
        return cpu_signals + gpu_signals
    else:
        return cpu_signals


def trace_signals():
    cpu_signals = ["CPU_POWER@package", "DRAM_POWER@package", "CPU_FREQUENCY_STATUS@package",
                   "CPU_PACKAGE_TEMPERATURE@package", "MSR::UNCORE_PERF_STATUS:FREQ@package",
                   "MSR::QM_CTR_SCALED_RATE@package", "CPU_INSTRUCTIONS_RETIRED@package",
                   "CPU_CYCLES_THREAD@package", "CPU_ENERGY@package",
                   "MSR::APERF:ACNT@package",
                   "MSR::MPERF:MCNT@package", "MSR::PPERF:PCNT@package"]
    gpu_signals = ["GPU_CORE_FREQUENCY_STATUS@gpu", "GPU_POWER@gpu", "GPU_UTILIZATION@gpu",
                   "GPU_CORE_ACTIVITY@gpu", "GPU_UNCORE_ACTIVITY@gpu"]

    if machine.num_gpu() > 0:
        return cpu_signals + gpu_signals
    else:
        return cpu_signals

def main(app_conf, **defaults):
    parser = argparse.ArgumentParser()
    setup_run_args(parser)
    parser.set_defaults(**defaults)
    args, extra_cli_args = parser.parse_known_args()
    launch(app_conf=app_conf, args=args,
           experiment_cli_args=extra_cli_args)
