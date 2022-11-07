#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and provides that as part of a constconfig file for the GPU Activity Agent
'''

import argparse
import code
import json
import jsonschema
import math
import os
import sys

import pandas
import numpy as np

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

def efficient_energy(df):
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
    gpu_freq_efficient = efficient_energy(df)

    json_dict = {
                    "GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY" : {
                        "domain" : "board",
                        "description" : "Defines the efficient compute frequency to use for GPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [gpu_freq_efficient],
                    },
                }

    return json_dict

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

    root_dir = os.getenv('GEOPM_SOURCE')
    schema_file = root_dir + "/service/json_schemas/const_config_io.schema.json"
    with open(schema_file, "r") as f:
        schema = json.load(f)

    jsonschema.validate(output, schema=schema)

    sys.stdout.write(json.dumps(output, indent=4) + "\n")
