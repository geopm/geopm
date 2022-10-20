#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and provides that as part of a policy for the GPU Activity Agent
'''

import code
import argparse
import math
import sys
import json

import pandas
import numpy as np
from numpy.polynomial.polynomial import Polynomial

import geopmpy.io

from experiment import common_args
from experiment import machine

def extract_columns(df):
    """
    Extract the columns of interest from the full report collection
    dataframe.
    """
    df_filtered = df

    # these are the only columns we need
    try:
        df_cols = df_filtered[['runtime (s)',
                                'package-energy (J)',
                                'dram-energy (J)',
                                'frequency (Hz)',
                                'gpu-frequency (Hz)',
                                'gpu-energy (J)']]
    except:
        df_cols = df_filtered[['runtime (s)',
                                'package-energy (J)',
                                'dram-energy (J)',
                                'frequency (Hz)',
                                'gpu-energy (J)']]
        df_cols['gpu-frequency (Hz)'] = df_filtered['GPU_CORE_FREQUENCY_STATUS']

    return df_cols

def policy_efficient_energy(df):
    """
    Find the frequency that provides minimum gpu energy consumption within the
    dataframe provided
    """

    energy_efficient_frequency = df.groupby('gpu-frequency (Hz)')['gpu-energy (J)'].mean().idxmin()
    return energy_efficient_frequency

def main(full_df):
    """
    The main function. full_df is a report collection dataframe
    """
    df = extract_columns(full_df)
    gpu_freq_efficient = policy_efficient_energy(df)
    # This script is not intended to provide an assessment of maximum frequency,
    # or phi.  As such NAN is provided for these policy values.
    # The associated agent interprets these as the related system or agent default
    # config, ex: GPU_FREQ_MAX: NAN --> HW Maximum Frequency, GPU_PHI --> 0.5
    # (balanced mode).
    policy = {"GPU_FREQ_MAX" : float('nan'),
              "GPU_FREQ_EFFICIENT" : gpu_freq_efficient,
              "GPU_PHI" : float('nan')}

    return policy

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    args = parser.parse_args()

    try:
        df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_app_df()
    except RuntimeError:
        sys.stderr.write('<geopm> Error: No report data found in ' + path + \
                         '; run a frequency sweep before using this analysis.\n')
        sys.exit(1)

    output = main(df)
    sys.stdout.write("AGENT POLICY:\n")
    sys.stdout.write(json.dumps(output) + "\n")
