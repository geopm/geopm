#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and characterizes the system memory bandwidth usage for the CPU Activity Agent
as a constconfig file.
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

from experiment import common_args
from experiment import machine

def extract_columns(df, region_list = None):
    """
    Extract the columns of interest from the full report collection
    dataframe.
    """
    df_filtered = df
    if region_list:
        df_filtered = df.loc[df['region'].isin(region_list)]

    if ('MSR::QM_CTR_SCALED_RATE' not in df_filtered.columns):
        df_filtered['MSR::QM_CTR_SCALED_RATE'] = (df_filtered['MSR::QM_CTR_SCALED_RATE@package-0'] +
                                             df_filtered['MSR::QM_CTR_SCALED_RATE@package-1'])/2

    if ('uncore-frequency (Hz)' not in df_filtered.columns):
        df_filtered['uncore-frequency (Hz)'] = (df_filtered['CPU_UNCORE_FREQUENCY_STATUS@package-0'] +
                                                df_filtered['CPU_UNCORE_FREQUENCY_STATUS@package-1'])/2

    # these are the only columns we need
    df_cols = df_filtered[['region',
                            'runtime (s)',
                            'package-energy (J)',
                            'dram-energy (J)',
                            'frequency (Hz)',
                            'MSR::QM_CTR_SCALED_RATE',
                            'uncore-frequency (Hz)',]]

    return df_cols

def analyze_efficient_energy(df, freq_col_name):
    """
    Find the frequency that provides the minimum package energy consumption
    within the dataframe provided.
    """

    energy_efficient_frequency = df.groupby(freq_col_name)['package-energy (J)'].mean().idxmin()
    return energy_efficient_frequency

def system_memory_bandwidth_characterization(df_region_group, region):
    """
    Perform characterization of the system memory bandwidth.  This
    is used by the agent to help determine the appropriate uncore frequency
    """
    df = df_region_group.get_group(region)
    uncore_freq_set = sorted(set(df['uncore-frequency (Hz)'].to_list()))

    mem_bw_dict = {}
    for k in uncore_freq_set:
        mem_df = df.groupby('uncore-frequency (Hz)').get_group(k)
        #TODO: use both packages!  This will skew in favor of package-0
        mem_bw_dict[k] = mem_df['MSR::QM_CTR_SCALED_RATE'].mean()

    return mem_bw_dict

def frequency_recommendation(df_region_group, region, domain):
    if domain == "UNCORE":
        freq_col = 'uncore-frequency (Hz)'
        fixed_freq_col = 'frequency (Hz)'
    elif domain == "CORE":
        freq_col = 'frequency (Hz)'
        fixed_freq_col = 'uncore-frequency (Hz)'
    else:
        sys.stderr.write('<geopm> Error: unsupported domain ' + domain + \
                         'provided\n')
        sys.exit(1)

    # Start with a region analysis for energy efficiency
    df = df_region_group.get_group(region)
    domain_freq_efficient = analyze_efficient_energy(df, freq_col)

    return domain_freq_efficient

def main(full_df, region_list):
    """
    The main function. full_df is a report collection dataframe, region_list
    is a list of regions to include.
    """
    df = extract_columns(full_df, region_list)

    # Handle any frequency clipping via rounding to the nearest 100Mhz
    # An alternative would be to use the FREQ_DEFAULT and FREQ_UNCORE
    # values from the policy
    df['frequency (Hz)'] = (df['frequency (Hz)']/1e09).round(decimals=1)*1e09
    df['uncore-frequency (Hz)'] = (df['uncore-frequency (Hz)']/1e09).round(decimals=1)*1e09
    df_region_group = df.groupby('region')

    # Characterize MBM metrics using the uncore sensitive
    # region (intensity_1)
    mem_bw_characterization = system_memory_bandwidth_characterization(df_region_group, region=region_list[0])

    # A multi-step approach is used.
    # First analyze the uncore sensitive
    # region (intensity_1) to find the efficient
    # uncore frequency
    uncore_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[0],
                                                           domain="UNCORE")

    # Then analyze the core senstivite region (intensity_16)
    # to find the most efficient core frequency
    # when running at the uncore_freq_efficient determined
    # above
    df = df[df['uncore-frequency (Hz)'] == uncore_freq_recommendation]
    df_region_group = df.groupby('region')
    core_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[1],
                                                         domain="CORE")

    json_dict = {
                    "CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY" : {
                        "domain" : "board",
                        "description" : "Defines the efficient core frequency to use for CPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain.",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [core_freq_recommendation],
                    },
                    "CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY": {
                        "domain" : "board",
                        "description" : "Defines the efficient uncore frequency to use for CPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [uncore_freq_recommendation],
                    }
                }

    for idx, (k,v) in enumerate(mem_bw_characterization.items()):
        json_dict["CPU_UNCORE_FREQUENCY_" + str(idx)] = {"domain" : "board",
                                                        "description" : "CPU Uncore Frequency associated with " +
                                                                        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + str(idx),
                                                        "units" : "hertz",
                                                        "aggregation" : "average",
                                                        "values" : [k]}
        json_dict["CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + str(idx)] = {"domain" : "board",
                                                                    "description" : "Maximum memory bandwidth in " +
                                                                                    "bytes perf second " +
                                                                                    "associated with " +
                                                                                    "CPU_UNCORE_FREQUENCY_" + str(idx),
                                                                    "units" : "none",
                                                                    "aggregation" : "average",
                                                                    "values" : [v]}
    return json_dict

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--region-list', default="intensity_1,intensity_16", dest='region_list',
                        help='comma-separated list of the two regions to use, '
                             'with the first used for uncore frequency characterization '
                             'and the second used for core frequency characteriztaion.  '
                             'Default is intensity_1,intensity_16')
    args = parser.parse_args()

    region_list = args.region_list.split(',')
    if len(region_list) != 2:
        sys.stderr.write('<geopm> Error: Exactly two regions are required'\
                         'for this analysis.\n')
        sys.exit(1)

    try:
        df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_df()
    except RuntimeError:
        sys.stderr.write('<geopm> Error: No report data found in ' + path + \
                         '; run a power sweep before using this analysis.\n')
        sys.exit(1)

    output = main(df, region_list)

    root_dir = os.getenv('GEOPM_SOURCE')
    schema_file = root_dir + "/service/json_schemas/const_config_io.schema.json"
    with open(schema_file, "r") as f:
        schema = json.load(f)

    jsonschema.validate(output, schema=schema)

    sys.stdout.write(json.dumps(output, indent=4) + "\n")
