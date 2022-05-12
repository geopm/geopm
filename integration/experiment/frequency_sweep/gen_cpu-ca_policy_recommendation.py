#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Creates a model relating power-limit to both runtime and energy
consumption. Offers power-governor policy recommendations for
minimum energy use subject to a runtime degradation constraint.
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

def extract_columns(df, region_filter = None):
    """
    Extract the columns of interest from the full report collection
    dataframe. This returns a dataframe indexed by the power limit
    and columns 'runtime' and 'energy'. region_filter (if provided)
    is a container that specifies which regions to include (by default,
    include all of them)."""
    df_filtered = df
    if region_filter:
        df_filtered = df[df['region'].isin(region_filter.split(','))]

    # these are the only columns we need
    try:
        df_cols = df_filtered[['region',
                                'runtime (s)',
                                'package-energy (J)',
                                'dram-energy (J)',
                                'frequency (Hz)',
                                'uncore-frequency (Hz)',]]
    except:
        df_cols = df_filtered[['region',
                                'runtime (s)',
                                'package-energy (J)',
                                'dram-energy (J)',
                                'frequency (Hz)']]

        df_cols['uncore-frequency (Hz)'] = (df_filtered['MSR::UNCORE_PERF_STATUS:FREQ@package-0'] +
                                            df_filtered['MSR::UNCORE_PERF_STATUS:FREQ@package-1'])/2
    return df_cols

def policy_efficient_energy(df, tolerance, domain):
    """
    Find the power limit over the range plrange (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of PowerLimitModel), subject to the constraint
    that its runtime does not exceed the runtime at power limit pltdp
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified. Returns a dictionary with keys power, runtime, and energy,
    and values the optimal power limit and the predicted runtime and energy
    at that limit, respectively."""

    if domain == "UNCORE":
        freq_col = 'uncore-frequency (Hz)'
    elif domain == "CORE":
        freq_col = 'frequency (Hz)'
    else:
        sys.stderr.write('<geopm> Error: unsupported domain ' + domain + \
                         'proovided\n')
        sys.exit(1)

    energy_min = df['package-energy (J)'].min()
    energy_max = df['package-energy (J)'].max()
    energy_min_normalized = energy_min/energy_max
    df['package-energy-normalized (%)'] = df['package-energy (J)'] / energy_max
    energy_within_tolerance = [e for e in df['package-energy-normalized (%)']
                                if e < energy_min_normalized + tolerance]
    energy_efficient_frequency = float(df[df['package-energy-normalized (%)'] ==
                                        energy_within_tolerance[0]][freq_col])
    recommended_frequency = energy_efficient_frequency

    return recommended_frequency

def policy_perf_deg(df, tolerance, domain):
    #if domain == "UNCORE":
    #    freq_col = 'uncore-frequency (Hz)'
    #elif domain == "CORE":
    #    freq_col = 'frequency (Hz)'
    #else:
    #    sys.stderr.write('<geopm> Error: unsupported domain ' + domain + \
    #                     'proovided\n')
    #    sys.exit(1)

    #if max_degradation is not None:
    #    runtime_min = df['runtime (s)'].min()
    #    runtime_max = df['runtime (s)'].max()
    #    df['runtime-normalized (%)'] = df['runtime (s)'] / runtime_max
    #    runtime_min_normalized = runtime_min / runtime_max

    #    runtime_within_tolerance = [r for r in df['runtime-normalized (%)']
    #                                if r < runtime_min_normalized + max_degradation]
    #    perf_deg_frequency = float(df[df['runtime-normalized (%)'] ==
    #                                        runtime_within_tolerance[0]][freq_col])

    #    code.interact(local=locals())
    #    recommended_frequency = perf_deg_frequency
    pass

def main(full_df, region_filter, tolerance, min_energy, max_degradation):
    """
    The main function. full_df is a report collection dataframe, region_filter
    is a list of regions to include.
    """
    df = extract_columns(full_df, region_filter)
    df_region_group = df.groupby('region')
    # A two pass approoach is used, first analyze
    # the most uncore sensitive region (intensity_0)
    # to find the efficient uncore frequency
    # (or a lower frequency if tolerance is not 0%)
    df = df_region_group.get_group('intensity_0')
    uncore_freq_efficient = policy_efficient_energy(df, tolerance, "UNCORE")
    uncore_freq_max = 2.4e9 #system max
    #TODO find max intensity_32 perf deg within bounds, may replace tolerance calc

    # Then analyze the most core senstivite region (intensity_32)
    # to find the most efficient core frequency (or lower, depending
    # on tolerance) when running at the uncore_freq_efficient determined
    # above
    df = df_region_group.get_group('intensity_32')
    df = df.groupby('uncore-frequency (Hz)').get_group(uncore_freq_efficient)
    #TODO find max intensity_0 perf deg within bounds, may replace tolerance calc

    core_freq_efficient = policy_efficient_energy(df, tolerance, "CORE")
    core_freq_max = 3.7e9 #system max

    policy = {"CPU_FREQ_MAX" : core_freq_max,
                "CPU_FREQ_EFFICIENT" : core_freq_efficient,
                "UNCORE_FREQ_MAX" : uncore_freq_max,
                "UNCORE_FREQ_EFFICIENT" : uncore_freq_efficient,
                "CPU_PHI" : 0.5,
                "SAMPLE_PERIOD" : 0.1}
    #CPU_FREQ_MAX,CPU_FREQ_EFFICIENT,UNCORE_FREQ_MAX,UNCORE_FREQ_EFFICIENT,CPU_PHI,SAMPLE_PERIOD
    return policy

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--max-degradation',
                        default=None, type=float, dest='max_degradation',
                        help='maximum allowed runtime degradation, default is '
                             '0.1 (i.e., 10%%)')
    parser.add_argument('--min_energy', action='store_true', dest='min_energy', default=True,
                        help='ignore max degradation, just give the minimum '
                             'energy possible')
    args = parser.parse_args()

    try:
        df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_df()
        #if args.region_filter == 'Epoch':
        #    df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_epoch_df()
        #else:
        #    df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_df()
    except RuntimeError:
        sys.stderr.write('<geopm> Error: No report data found in ' + path + \
                         '; run a power sweep before using this analysis.\n')
        sys.exit(1)

    tolerance = 0.05
    region_filter = "intensity_0,intensity_32"
    output = main(df, region_filter, tolerance, args.min_energy, args.max_degradation)

    sys.stdout.write("POLICY: {}\n".format(output))
    #CPU_FREQ_MAX,CPU_FREQ_EFFICIENT,UNCORE_FREQ_MAX,UNCORE_FREQ_EFFICIENT,CPU_PHI,SAMPLE_PERIOD

    #if args.confidence:
    #    sys.stdout.write('AT TDP = {power:.0f}W, '
    #                     'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
    #                     'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['tdp']))
    #    if output['best']:
    #        sys.stdout.write('AT PL  = {power:.0f}W, '
    #                         'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
    #                         'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['best']))
    #    else:
    #        sys.stdout.write("NO SUITABLE POLICY WAS FOUND.\n")
    #else:
    #    sys.stdout.write('AT TDP = {power:.0f}W, '
    #                     'RUNTIME = {runtime:.0f} s, '
    #                     'ENERGY = {energy:.0f} J\n'.format(**output['tdp']))
    #    sys.stdout.write('AT PL  = {power:.0f}W, '
    #                     'RUNTIME = {runtime:.0f} s, '
    #                     'ENERGY = {energy:.0f} J\n'.format(**output['best']))

    #relative_delta = lambda new, old: 100 * (new - old) / old

    #if output['best']:
    #    sys.stdout.write('DELTA          RUNTIME = {:.1f} %,  ENERGY = {:.1f} %\n'
    #                     .format(relative_delta(output['best']['runtime'], output['tdp']['runtime']),
    #                             relative_delta(output['best']['energy'], output['tdp']['energy'])))
