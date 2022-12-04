#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and characterizes the system memory bandwidth usage for the CPU Activity Agent
as a ConstConfigIO configuration file.
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

from experiment import util
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
                            'uncore-frequency (Hz)']]

    return df_cols

def energy_efficient_frequency(df, freq_col_name, energy_margin):
    """
    Find the frequency that provides the minimum package energy consumption
    within the dataframe provided.
    """
    df_mean = df.groupby(freq_col_name)['package-energy (J)'].mean()
    energy_efficient_frequency = df_mean.idxmin()
    min_energy = df_mean[energy_efficient_frequency];

    if len(df_mean) > 1:
        if energy_margin != 0.0:
            sys.stderr.write('Found Fe = {} with energy = {}.  Searching for alternate '
                             'based on an energy margin of {}\n'.format(energy_efficient_frequency,min_energy, energy_margin))

            # Grab all energy readings associated with frequencies that a 1Hz below Fe
            # TODO: Consider iloc instead and just grab all idx prior to Fe
            df_mean = df_mean.loc[:energy_efficient_frequency - 1]

            # Find any energy reading that is within 5% of Fe's energy while having a lower frequency
            min_energy = max([e for e in df_mean if (e - min_energy) / e < energy_margin]);
            # Store the associated frequency
            energy_efficient_frequency = df_mean[df_mean == min_energy].index[0];
            sys.stderr.write('Found alternate Fe = {} with energy = {}.\n'.format(energy_efficient_frequency,min_energy))
        else:
            df_mean = df_mean.loc[:energy_efficient_frequency - 1]
            nearby_energy_count = len([e for e in df_mean if (e - min_energy) / e < 0.05]);
            sys.stderr.write('Warning: Found {} possible alternate Fe value(s) within 5% '
                             'energy consumption of Fe for \'{}\'.  Consider using the core or uncore energy-margin options.\n'.format(nearby_energy_count, freq_col_name))

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

def frequency_recommendation(df_region_group, region, domain, energy_margin):
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
    domain_freq_efficient = energy_efficient_frequency(df, freq_col, energy_margin)

    return domain_freq_efficient

def get_config_from_frequency_sweep(full_df, region_list, core_energy_margin, uncore_energy_margin):
    """
    The main function. full_df is a report collection dataframe, region_list
    is a list of regions to include.
    """
    df = extract_columns(full_df, region_list)
    df_region_group = df.groupby('region')

    # Characterize MBM metrics using the uncore sensitive
    # region (intensity_1)
    mem_bw_characterization = system_memory_bandwidth_characterization(df_region_group, region=region_list[0])

    # A multi-step approach is used.
    # First analyze the uncore sensitive
    # region (intensity_1) to find the efficient
    # uncore frequency
    uncore_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[0],
                                                           domain="UNCORE", energy_margin=uncore_energy_margin)

    # Then analyze the core senstivite region (intensity_16)
    # to find the most efficient core frequency
    # when running at the uncore_freq_efficient determined
    # above
    df = df[df['uncore-frequency (Hz)'] == uncore_freq_recommendation]
    df_region_group = df.groupby('region')
    core_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[1],
                                                         domain="CORE", energy_margin=core_energy_margin)

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
                                        "This value is based on a workload that scales strongly with the frequency domain.",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [uncore_freq_recommendation],
                    }
                }

    for idx, (k,v) in enumerate(mem_bw_characterization.items()):
        json_dict["CPU_UNCORE_FREQUENCY_" + str(idx)] = {"domain" : "board",
                                                        "description" : "CPU Uncore Frequency associated with " +
                                                                        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + str(idx) + ".",
                                                        "units" : "hertz",
                                                        "aggregation" : "average",
                                                        "values" : [k]}
        json_dict["CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + str(idx)] = {"domain" : "board",
                                                                    "description" : "Maximum memory bandwidth in " +
                                                                                    "bytes perf second " +
                                                                                    "associated with " +
                                                                                    "CPU_UNCORE_FREQUENCY_" + str(idx) + ".",
                                                                    "units" : "none",
                                                                    "aggregation" : "average",
                                                                    "values" : [v]}
    return json_dict

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--const-config-path', required=False, default=None,
                        help='path containing existing ConstConfigIO configuration file')
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--core-energy-margin', default=0, type=float, dest='core_energy_margin',
                        help='Percentage of acceptable additional energy that is acceptable if '
                             'a lower frequency is selected for Fe.  This is useful for analyzing '
                             'noisy systems that have many frequencies that are near the Fe point')
    parser.add_argument('--uncore-energy-margin', default=0, type=float, dest='uncore_energy_margin',
                        help='Percentage of acceptable additional energy that is acceptable if '
                             'a lower frequency is selected for Fe.  This is useful for analyzing '
                             'noisy systems that have many frequencies that are near the Fe point')
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

    output = get_config_from_frequency_sweep(df, region_list, args.core_energy_margin, args.uncore_energy_margin)
    output = util.merge_const_config(output, args.const_config_path);

    sys.stdout.write(json.dumps(output, indent=4) + "\n")
