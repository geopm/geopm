#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and provides that as part of a ConstConfigIO configuration file for the GPU Activity Agent
'''

import argparse
import json
import jsonschema
import math
import os
import sys

import pandas
import numpy as np

import geopmpy.io

from integration.experiment import util
from integration.experiment import common_args
from integration.experiment import machine

def extract_columns(df):
    """
    Extract the columns of interest from the full report collection
    dataframe.
    """

    #Explicitly a copy to deal with setting on copy errors
    df_filtered = df.copy()

    # Use requested frequency from the agent
    df_filtered['requested gpu-frequency (Hz)'] = df['FREQ_GPU_DEFAULT']

    # these are the only columns we need
    try:
        df_filtered = df_filtered[['runtime (s)',
                                'package-energy (J)',
                                'dram-energy (J)',
                                'frequency (Hz)',
                                'gpu-frequency (Hz)',
                                'gpu-energy (J)',
                                'requested gpu-frequency (Hz)']]

    except:
        df_filtered = df_filtered[['runtime (s)',
                                'package-energy (J)',
                                'dram-energy (J)',
                                'frequency (Hz)',
                                'gpu-energy (J)',
                                'requested gpu-frequency (Hz)']]

        df_filtered['gpu-frequency (Hz)'] = df_filtered['GPU_CORE_FREQUENCY_STATUS']

    return df_filtered

def get_config_from_frequency_sweep(full_df, mach, energy_margin, use_freq_req):
    """
    The main function. full_df is a report collection dataframe
    """

    df = extract_columns(full_df)

    #Round entries to nearest step size
    frequency_step = mach.gpu_frequency_step()
    df.loc[:,'gpu-frequency (Hz)'] = (df['gpu-frequency (Hz)'] /
                                         frequency_step).round(decimals=0) * frequency_step

    energy_col = 'gpu-energy (J)'
    if use_freq_req:
        freq_col = 'requested gpu-frequency (Hz)'
    else:
        freq_col = 'gpu-frequency (Hz)'

    gpu_freq_efficient = util.energy_efficient_frequency(df, freq_col, energy_col, energy_margin)

    json_dict = {
                    "GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY" : {
                        "domain" : "board",
                        "description" : "Defines the efficient compute frequency to use for GPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain.",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [gpu_freq_efficient],
                    },
                }

    return json_dict

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--const-config-path', required=False, default=None,
                        help='path containing existing ConstConfigIO configuration file')
    parser.add_argument('--gpu-energy-margin', default=0, type=float, dest='gpu_energy_margin',
                        help='Percentage of additional energy it is acceptable to consume if it results '
                             'in a lower frequency selection for Fe (energy efficient frequency).  This is useful for analyzing '
                             'noisy systems that have many GPU frequencies near the Fe energy consumption value')
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--use-requested-frequency', action='store_true', default=False,
                        dest='use_freq_req',
                        help='Use the frequency that was requested during the frequency sweep instead '
                             'of the achieved frequency for a given run.  This is useful in cases where '
                             'multiple frequency domains or settings are impacted (i.e. core frequency causes '
                             'an uncore frequency change) and the achieved frequency does not reflect this '
                             'behavior.')
    args = parser.parse_args()

    try:
        df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_app_df()
    except RuntimeError:
        sys.stderr.write('Error: <geopm> gen_gpu_activity_constconfig_recommendation.py: No report data found in ' + path + \
                         '; run a frequency sweep before using this analysis.\n')
        sys.exit(1)

    if args.gpu_energy_margin < 0:
        sys.stderr.write('Error: <geopm> gen_gpu_activity_constconfig_recommendation.py: GPU energy margin must be non-negative\n')
        sys.exit(1)

    mach = machine.get_machine(args.path);
    output = get_config_from_frequency_sweep(df, mach, args.gpu_energy_margin, args.use_freq_req)
    output = util.merge_const_config(output, args.const_config_path);

    sys.stdout.write(json.dumps(output, indent=4) + "\n")
