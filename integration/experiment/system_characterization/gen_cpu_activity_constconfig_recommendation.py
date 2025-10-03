#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and characterizes the system memory bandwidth usage for the CPU Activity Agent
as a ConstConfigIO configuration file.
'''

import argparse
import json
import os
import sys

import pandas
import numpy as np

import geopmpy.io

from integration.experiment import util
from integration.experiment import common_args
from integration.experiment import machine

def extract_columns(df, region_list = None):
    """
    Extract the columns of interest from the full report collection
    dataframe.
    """

    # Explicitly a copy to avoid SettingWithCopy errors in pandas
    if region_list:
        df_filtered = df.loc[df['region'].isin(region_list)].copy()
    else:
        df_filtered = df.copy()

    # These cases are intended to handle older runs where uncore-frequency is not
    # part of the report or the QM_CTR_SCALED_RATE has not been collected at the
    # board level, and should be removed at a later date

    if 'MSR::QM_CTR_SCALED_RATE' not in df_filtered.columns:
        df_filtered['MSR::QM_CTR_SCALED_RATE'] = (df_filtered['MSR::QM_CTR_SCALED_RATE@package-0'] +
                                                  df_filtered['MSR::QM_CTR_SCALED_RATE@package-1'])/2

    if 'uncore-frequency (Hz)' not in df_filtered.columns:
        df_filtered['uncore-frequency (Hz)'] = (df_filtered['CPU_UNCORE_FREQUENCY_STATUS@package-0'] +
                                                df_filtered['CPU_UNCORE_FREQUENCY_STATUS@package-1'])/2

    # Use requested frequency from the agent
    df_filtered['requested core-frequency (Hz)'] = df['CPU_FREQUENCY_MIN_CONTROL@package-0']
    df_filtered['requested uncore-frequency (Hz)'] = df['CPU_UNCORE_FREQUENCY_MIN_CONTROL@package-0']

    # these are the only columns we need
    df_filtered = df_filtered[['region',
                               'runtime (s)',
                               'package-energy (J)',
                               'dram-energy (J)',
                               'frequency (Hz)',
                               'MSR::QM_CTR_SCALED_RATE',
                               'uncore-frequency (Hz)',
                               'requested core-frequency (Hz)',
                               'requested uncore-frequency (Hz)',]]

    return df_filtered

def system_memory_bandwidth_characterization(df_region_group, region, freq_col):
    """
    Perform characterization of the system memory bandwidth.  This
    is used by the agent to help determine the appropriate uncore frequency
    """
    df = df_region_group.get_group(region)
    uncore_freq_set = sorted(set(df[freq_col].to_list()))

    mem_bw_dict = {}
    for k in uncore_freq_set:
        mem_df = df.groupby(freq_col).get_group(k)
        mem_bw_dict[k] = mem_df['MSR::QM_CTR_SCALED_RATE'].mean()

    return mem_bw_dict

def frequency_recommendation(df_region_group, region, freq_col, energy_margin):
    energy_col = 'package-energy (J)'

    # Start with a region analysis for energy efficiency
    df = df_region_group.get_group(region)
    domain_freq_efficient = util.energy_efficient_frequency(df, freq_col, energy_col, energy_margin)

    return domain_freq_efficient

def get_config_from_frequency_sweep(full_df, region_list, mach,
                                    core_energy_margin, uncore_energy_margin,
                                    use_freq_req, hostname):
    """
    The main function. full_df is a report collection dataframe, region_list
    is a list of regions to include.
    """
    df = extract_columns(full_df, region_list)

    if df['frequency (Hz)'].isna().any() or df['uncore-frequency (Hz)'].isna().any():
        raise RuntimeError('NaNs detected in the input report files')


    #Round entries to nearest step size
    frequency_step = mach.frequency_step()
    df['frequency (Hz)'] = (df['frequency (Hz)'] / frequency_step).round(decimals=0) * frequency_step
    df['uncore-frequency (Hz)'] = (df['uncore-frequency (Hz)'] / frequency_step).round(decimals=0) * frequency_step

    # A multi-step approach is used.

    # Step-1: analyze the core senstivite region (ex: intensity_16)
    # to find the most efficient core frequency

    #  Step-1a: Initialization
    df_region_group = df.groupby('region')
    if use_freq_req:
        freq_col = 'requested core-frequency (Hz)'
    else:
        freq_col = 'frequency (Hz)'

    # Step-1b: Calculate efficient core frequency value
    core_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[1],
                                                        freq_col=freq_col, energy_margin=core_energy_margin)

    # Step-1c: Round to step size
    core_freq_recommendation = round(core_freq_recommendation / frequency_step) * frequency_step


    # Step-2: adjust dataset to only include entries that match the 
    # newly calculated efficient core frequency value
    df = df[df['frequency (Hz)'] == core_freq_recommendation]


    # Step-3: analyze the uncore sensitive
    # region (ex: intensity_1) to find the efficient
    # uncore frequency when running at the core_freq_recommendation
    # determined above.

    # Step 3a: Initialization
    df_region_group = df.groupby('region')
    if use_freq_req:
        freq_col = 'requested uncore-frequency (Hz)'
    else:
        freq_col = 'uncore-frequency (Hz)'

    # Step-3b-i: Characterize MBM metrics using the uncore sensitive region (intensity_1)
    mem_bw_characterization = system_memory_bandwidth_characterization(df_region_group,
                                                                       region=region_list[0], freq_col=freq_col)

    # Step-3b-ii: Calculate efficient uncore frequency value
    uncore_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[0],
                                                          freq_col=freq_col, energy_margin=uncore_energy_margin)

    # Step-3c: Round to step size
    uncore_freq_recommendation = round(uncore_freq_recommendation / frequency_step) * frequency_step

    ## Populate the characterization info in JSON format

    if not hostname:
        host_str = ''
    else:
        host_str = '@' + hostname

    json_dict = {
                    ("CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY" + host_str) : {
                        "domain" : "board",
                        "description" : "Defines the efficient core frequency to use for CPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain.",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [core_freq_recommendation],
                    },
                    ("CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY" + host_str): {
                        "domain" : "board",
                        "description" : "Defines the efficient uncore frequency to use for CPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain.",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [uncore_freq_recommendation],
                    }
                }

    for idx, (k,v) in enumerate(mem_bw_characterization.items()):
        json_dict["CPU_UNCORE_FREQUENCY_" + str(idx) + host_str] = {
                                                        "domain" : "board",
                                                        "description" : "CPU Uncore Frequency associated with " +
                                                                        "CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + str(idx) + ".",
                                                        "units" : "hertz",
                                                        "aggregation" : "average",
                                                        "values" : [k]}
        json_dict["CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" + str(idx) + host_str] = {
                                                                    "domain" : "board",
                                                                    "description" : "Maximum memory bandwidth in " +
                                                                                    "bytes per second " +
                                                                                    "associated with " +
                                                                                    "CPU_UNCORE_FREQUENCY_" + str(idx) + ".",
                                                                    "units" : "none",
                                                                    "aggregation" : "average",
                                                                    "values" : [v]}
    return json_dict

def main():
    try:
        parser = argparse.ArgumentParser()
        parser.add_argument('--const-config-path', required=False, default=None, dest='const_config_path',
                            help='path to the existing ConstConfigIO configuration file')
        parser.add_argument('--input-path', required=True, dest='input_path',
                            help='path containing reports and machine.json')
        parser.add_argument('--output-path', required=False, default='-', dest='output_path',
                            help='output directory to dump node characterization info to')
        parser.add_argument('--hostname', required=True, default=None, dest='hostname',
                            help='hostname of the node being characterized')
        parser.add_argument('--core-energy-margin', default=0, type=float, dest='core_energy_margin',
                            help='Percentage of additional energy it is acceptable to consume if it results '
                                 'in a lower frequency selection for Fe (energy efficient frequency).  This is useful for analyzing '
                                 'noisy systems that have many core frequencies near the Fe energy consumption value')
        parser.add_argument('--uncore-energy-margin', default=0, type=float, dest='uncore_energy_margin',
                            help='Percentage of additional energy it is acceptable to consume if it results '
                                 'in a lower frequency selection for Fe.  This is useful for analyzing '
                                 'noisy systems that have many uncore frequencies near the Fe energy consumption value.')
        parser.add_argument('--region-list', default="intensity_1,intensity_16", dest='region_list',
                            help='comma-separated list of the two regions to use, '
                                 'with the first used for uncore frequency characterization '
                                 'and the second used for core frequency characterization.  '
                                 'Default is intensity_1,intensity_16')
        parser.add_argument('--use-requested-frequency', action='store_true', default=False,
                            dest='use_freq_req',
                            help='Use the frequency that was requested during the frequency sweep instead '
                                 'of the achieved frequency for a given run.  This is useful in cases where '
                                 'multiple frequency domains or settings are impacted (i.e. core frequency causes '
                                 'an uncore frequency change) and the achieved frequency does not reflect this '
                                 'behavior.')
        parser.add_argument('--boardcap', required=True, dest='boardcap',
                            action='store', default=False,
                            help='Platform-level board power cap')

        args = parser.parse_args()

        region_list = args.region_list.split(',')
        if len(region_list) != 2:
            raise RuntimeError('Exactly two regions are required')

        df = geopmpy.io.RawReportCollection('*boardcap_' + args.boardcap + '*cpufreqsweep*' + args.hostname + '*report', dir_name=args.input_path).get_df()

        if args.uncore_energy_margin < 0 or args.core_energy_margin < 0:
            raise RuntimeError('Core & Uncore energy margin must be non-negative')

        mach = machine.get_machine(args.input_path);

        output = get_config_from_frequency_sweep(df, region_list, mach,
                                                 args.core_energy_margin,
                                                 args.uncore_energy_margin,
                                                 args.use_freq_req, args.hostname)
        output = util.merge_const_config(output, args.const_config_path)

        output_str =  json.dumps(output, indent=4)
        if args.output_path != '-':
            constconfig_outfile=f'{args.output_path}/{args.hostname}.json'
            with open(constconfig_outfile, 'w') as file:
                file.writelines(output_str)
        else:
            print(output_str)
    except Exception as ex:
        if 'GEOPM_DEBUG' in os.environ:
            raise
        sys.stderr.write(f'Error: <geopm> gen_cpu_activity_constconfig_recommendation.py: {ex} + \n\n')
        return -1
    return 0

if __name__ == "__main__":
    sys.exit(main())
