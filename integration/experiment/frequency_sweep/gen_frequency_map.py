#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Calculates the best-fit frequencies for each region in a frequency for
a given maximum performance degradation (runtime increase).
'''

import sys
import argparse
import json

import geopmpy.io
import geopmpy.hash

from integration.experiment import common_args


def find_optimal_freq(region_df, perf_margin):
    freqs = sorted(region_df['freq_hz'].unique(), reverse=True)
    is_once = True
    for freq in freqs:
        freq_df = region_df.loc[region_df['freq_hz'] == freq]
        runtime = freq_df['runtime'].mean()
        if is_once:
            min_runtime = runtime
            optimal_freq = freq
        elif min_runtime > runtime:
            min_runtime = runtime
            optimal_freq = freq
        elif min_runtime * (1.0 + perf_margin) > runtime:
            optimal_freq = freq
        is_once = False
    return optimal_freq


def frequency_map(report_df, perf_margin, show_details):
    # rename some columns
    # TODO: only works for freq map agent
    report_df['freq_hz'] = report_df['FREQ_CPU_DEFAULT']
    report_df['runtime'] = report_df['runtime (sec)']
    report_df['network_time'] = report_df['network-time (sec)']
    report_df['energy_pkg'] = report_df['package-energy (joules)']
    report_df['frequency'] = report_df['frequency (Hz)']
    report_df = report_df[['freq_hz', 'runtime', 'region', 'network_time',
                           'energy_pkg', 'frequency', 'count']]

    if show_details:
        sys.stdout.write('data: {}\n'.format(report_df))

    result = {}
    regions = report_df['region'].unique()
    for region in regions:
        region_df = report_df[report_df['region'] == region]
        if show_details:
            sys.stdout.write('Data for {}:\n{}\n'.format(region, report_df))
        result[region] = find_optimal_freq(region_df, perf_margin)
    return result


def format_region_freq_map(df, perf_margin, show_details):
    freq_map = frequency_map(df, perf_margin, show_details)
    sys.stdout.write('Best-fit frequencies with {}% performance degradation:\n'.format(100 * perf_margin))
    sys.stdout.write('{}\n'.format(freq_map))
    max_freq = df['FREQ_CPU_DEFAULT'].max()
    sys.stdout.write('\nFrequency map agent policy:\n')

    idx = 0
    policy = {"FREQ_CPU_DEFAULT": max_freq}
    for region_name in df['region'].unique():
        region_hash = geopmpy.hash.hash_str(region_name)
        region_freq = freq_map[region_name]
        policy['HASH_{}'.format(idx)] = region_hash
        policy['FREQ_{}'.format(idx)] = region_freq
        idx += 1
    sys.stdout.write('{}\n'.format(json.dumps(policy)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    parser.add_argument('--perf-margin', dest='perf_margin',
                        action='store', type=float, default=0.10,
                        help='fraction of performance degradation from max frequency to allow (default=0.10)')
    args = parser.parse_args()
    perf_margin = args.perf_margin
    if perf_margin < 0 or perf_margin > 1.0:
        raise RuntimeError('Value for --perf-margin should be between 0.0 and 1.0')

    output_dir = args.output_dir
    show_details = args.show_details

    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    summary = format_region_freq_map(output.get_df(), perf_margin, show_details)
