#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and characterizes the system memory bandwidth usage for the CPU Activity Agent.
'''

import code
import argparse
import math
import sys

import pandas
import numpy as np
from numpy.polynomial.polynomial import Polynomial

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
        df_filtered = df[df['region'].isin(region_list)]

    if ('QM_CTR_SCALED_RATE' not in df_filtered.columns):
        df_filtered['QM_CTR_SCALED_RATE'] = (df_filtered['QM_CTR_SCALED_RATE@package-0'] +
                                             df_filtered['QM_CTR_SCALED_RATE@package-1'])/2

    if ('uncore-frequency (Hz)' not in df_filtered.columns):
        df_filtered['uncore-frequency (Hz)'] = (df_filtered['MSR::UNCORE_PERF_STATUS:FREQ@package-0'] +
                                                df_filtered['MSR::UNCORE_PERF_STATUS:FREQ@package-1'])/2

    # these are the only columns we need
    df_cols = df_filtered[['region',
                            'runtime (s)',
                            'package-energy (J)',
                            'dram-energy (J)',
                            'frequency (Hz)',
                            'QM_CTR_SCALED_RATE',
                            'uncore-frequency (Hz)',]]

    return df_cols

def analyze_efficient_energy(df, min_energy_tolerance, freq_col_name):
    """
    Find the frequency that provides the minimum package energy consumption
    within the dataframe provided.  If a tolerance is given, allow for
    frequency selection below the energy efficient frequency.
    """

    energy_min = df['package-energy (J)'].min()
    energy_max = df['package-energy (J)'].max()
    energy_min_normalized = energy_min/energy_max
    df['package-energy-normalized (%)'] = df['package-energy (J)'] / energy_max

    energy_within_tolerance = [e for e in df['package-energy-normalized (%)']
                                if e <= energy_min_normalized + min_energy_tolerance]

    # Mean is used here as there can be instances where multiple runs meet the criteria
    # and float cannot handle multiple values
    energy_efficient_frequency = float(df[df['package-energy-normalized (%)'] ==
                                       energy_min_normalized][freq_col_name].mean())
    energy_tolerant_frequency = float(df[df['package-energy-normalized (%)'] ==
                                      energy_within_tolerance[0]][freq_col_name].mean())

    return energy_efficient_frequency, energy_tolerant_frequency

def system_memory_bandwidth_characterization(df_region_group):
    """
    Perform characterization of the system memory bandwidth.  This
    is used by the agent to help determine the appropriate uncore frequency
    """
    df = df_region_group.get_group('intensity_0')
    uncore_freq_set = sorted(set(df['uncore-frequency (Hz)'].to_list()))

    mem_bw_dict = {}
    for k in uncore_freq_set:
        mem_df = df.groupby('uncore-frequency (Hz)').get_group(k)
        #TODO: use both packages!  This will skew in favor of package-0
        mem_bw_dict[k] = mem_df['QM_CTR_SCALED_RATE'].mean()

    return mem_bw_dict

def analyze_perf_deg(df, cross_region_degradation, freq_col_name):
    """
    Find the frequency in the dataframe that does comes closest to the
    specified performance degradation without exceeding it.  If no such
    value is found return the frequency associated with minimum runtime
    """

    runtime_min = df['runtime (s)'].min()
    runtime_max = df['runtime (s)'].max()
    df['perf-deg (%)'] = (df['runtime (s)'] / runtime_min - 1)

    perf_within_tolerance = [r for r in df['perf-deg (%)']
                            if r < cross_region_degradation]
    perf_deg_frequency = float(df[df['perf-deg (%)'] ==
                                perf_within_tolerance[0]][freq_col_name].mean())

    return perf_deg_frequency

def frequency_recommendation(df_region_group, region, cross_region, min_energy_tolerance, cross_region_degradation, domain):
    if domain == "UNCORE":
        freq_col = 'uncore-frequency (Hz)'
        fixed_freq_col = 'frequency (Hz)'
    elif domain == "CORE":
        freq_col = 'frequency (Hz)'
        fixed_freq_col = 'uncore-frequency (Hz)'
    else:
        sys.stderr.write('<geopm> Error: unsupported domain ' + domain + \
                         'proovided\n')
        sys.exit(1)

    # Start with a region analysis for energy efficiency
    df = df_region_group.get_group(region)
    domain_freq_efficient, domain_freq_tolerant = analyze_efficient_energy(df, min_energy_tolerance, freq_col)

    # Then do an analysis of the tolerant frequency
    # impact on the cross region.
    df = df_region_group.get_group(cross_region)

    # Fix the frequency of the domain not being searched (core)
    fixed_domain_freq = float(df[df['runtime (s)'] ==
                        df['runtime (s)'].min()][fixed_freq_col])
    df = df[df[fixed_freq_col] == fixed_domain_freq]

    # Build a reduced dataframe of just the experiments that used the
    # domain_freq_efficient and domain_freq_tolerant
    tolerant_df = pandas.concat([df[df[freq_col] == domain_freq_efficient],
                                df[df[freq_col] == domain_freq_tolerant]])

    # Run analysis on reduced dataframe to determine if the domain_freq_tolerant or
    # domain_freq_efficient is better for the cross region
    cross_domain_freq_efficient, _ = analyze_efficient_energy(tolerant_df, 0, freq_col)

    domain_freq_efficient = cross_domain_freq_efficient

    # Next do a cross region perf degradation analysis if requested
    if cross_region_degradation is not None and cross_region_degradation > 0.0:
        # Find the performance impact of decreasing
        # frequency below the efficient frequency for a region
        # that is NOT as performance sensitive to that frequency
        # domain.  Update efficient frequency based on this
        # analysis vs the specified perf degradation

        # Build a dataframe with the most performance option
        # for perf deg comparison and all frequency options
        # at or below the uncore efficient frequency.
        # Then perform the performance degradation analysis
        df = pandas.concat([df[df[freq_col] <= domain_freq_efficient],
                        df[df['runtime (s)'] == df['runtime (s)'].min()]])

        perf_deg_recommendation = analyze_perf_deg(df, cross_region_degradation, freq_col)

        # Compare the energy efficiency of the cross region performance degradation
        # to the previously found region uncore frequency efficient setting
        df = pandas.concat([df[df[freq_col]==domain_freq_efficient],
                            df[df[freq_col]==perf_deg_recommendation]])
        cross_region_efficient, _ = analyze_efficient_energy(df, min_energy_tolerance, freq_col)

        if perf_deg_recommendation < domain_freq_efficient and perf_deg_recommendation >= cross_region_efficient:
            domain_freq_efficient = perf_deg_recommendation

    return domain_freq_efficient

def main(full_df, region_list, min_energy_tolerance, cross_region_degradation):
    """
    The main function. full_df is a report collection dataframe, region_list
    is a list of regions to include.
    """
    df = extract_columns(full_df, region_list)
    df['frequency (Hz)'] = (df['frequency (Hz)']/1e09).round(decimals=1)*1e09
    df['uncore-frequency (Hz)'] = (df['uncore-frequency (Hz)']/1e09).round(decimals=1)*1e09
    df_region_group = df.groupby('region')

    # Handle any frequency clipping via rounding to the nearest 100Mhz
    # An alternative would be to use the FREQ_DEFAULT and FREQ_UNCORE
    # values from the policy

    # Characterize MBM metrics
    mem_bw_characterization = system_memory_bandwidth_characterization(df_region_group)

    # A multi-step approach is used.
    # First analyze the most uncore sensitive
    # region (intensity_0)  to find the efficient
    # uncore frequency (or a lower frequency if
    # min_energy_tolerance is not 0%)
    uncore_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[0],
                                                           cross_region=region_list[1], min_energy_tolerance=min_energy_tolerance,
                                                           cross_region_degradation=cross_region_degradation,
                                                           domain="UNCORE")

    # Then analyze the most core senstivite region (intensity_32)
    # to find the most efficient core frequency (or lower, depending
    # on min_energy_tolerance) when running at the uncore_freq_efficient determined
    # above
    df = df[df['uncore-frequency (Hz)'] == uncore_freq_recommendation]
    df_region_group = df.groupby('region')
    core_freq_recommendation = frequency_recommendation(df_region_group, region=region_list[1],
                                                         cross_region=region_list[0], min_energy_tolerance=min_energy_tolerance,
                                                         cross_region_degradation=cross_region_degradation,
                                                         domain="CORE")

    policy = {"CPU_FREQ_MAX" : float('nan'),
                "CPU_FREQ_EFFICIENT" : core_freq_recommendation,
                "UNCORE_FREQ_MAX" : float('nan'),
                "UNCORE_FREQ_EFFICIENT" : uncore_freq_recommendation,
                "CPU_PHI" : float('nan'),
                "SAMPLE_PERIOD" : float('nan')}

    for k,v in mem_bw_characterization.items():
        policy['MAX_MBM_UNCORE_FREQ_' + str(k)] = v

    return policy

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--cross-region-degradation',
                        default=0.00, type=float, dest='cross_region_degradation',
                        help='maximum allowed runtime degradation in the cross region '
                            'evaluation step.  NOTE: This is not the maximum possible '
                            'degradation for the policy default is 0.0 (i.e., 0%)')
    parser.add_argument('--min-energy-tolerance',
                        default=0.0, type=float, dest='min_energy_tolerance',
                        help='maximum allowed runtime degradation, default is '
                             '0.0 (i.e., 0%)')
    parser.add_argument('--region-list', default="intensity_0,intensity_32", dest='region_list',
                        help='comma-separated list of the two regions to use, '
                             'default is intensity_0,intensity_32')
    args = parser.parse_args()

    region_list = args.region_list.split(',')
    if len(region_list) != 2:
        sys.stderr.write('<geopm> Error: Exactly two regions are required'\
                         'for this analysis.\n')
        sys.exit(1)

    if args.min_energy_tolerance < 0:
        sys.stderr.write('<geopm> Error: min energy tolerance cannot be negative'\
                         'for this analysis.\n')
        sys.exit(1)

    if args.cross_region_degradation < 0:
        sys.stderr.write('<geopm> Error: cross region degradation cannot be negative'\
                         'for this analysis.\n')
        sys.exit(1)

    try:
        df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_df()
    except RuntimeError:
        sys.stderr.write('<geopm> Error: No report data found in ' + path + \
                         '; run a power sweep before using this analysis.\n')
        sys.exit(1)

    output = main(df, region_list, args.min_energy_tolerance, args.cross_region_degradation)

    sys.stdout.write("POLICY: {}\n".format(output))
