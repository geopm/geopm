#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import json
import pandas as pd
import argparse

_FREQUENCY_NORM_FACTOR=1e9

def get_domains(table_stats):
    domains = []
    for domain in ['cpu', 'gpu', 'uncore']:
        if f'{domain}-frequency' in table_stats.columns:
            domains.append(domain)
    return domains

def calc_linear_regression(x_df, y_df):
    if len(y_df) != len(x_df):
        sys.stderr.write("<geopm> Error: (gen_region_parameters) Attempting a"
                         "linear regression, mapping different size sets.\n")
        sys.exit(1)
    if len(y_df) == 0:
        sys.stderr.write("<geopm> Error: (gen_region_parameters) Attempting a"
                         "linear regression on an empty set.\n")
        sys.exit(1)

    slope = (len(y_df) * (y_df * x_df).sum() - y_df.sum() * x_df.sum())/(len(y_df) * (x_df * x_df).sum() - x_df.sum() * x_df.sum())

    intercept = y_df.mean() - slope * x_df.mean()

    return slope, intercept

def get_domain_freq_range(domain, table_stats):
    """Returns the min and max frequency for the given domain. If the domain
    is not present in the table, returns None."""

    if f'{domain}-frequency' in table_stats:
        return {'min_freq' : min(table_stats[f'{domain}-frequency']),
                'max_freq' : max(table_stats[f'{domain}-frequency'])}
    return None

#Outputs runtime = slope * inv_freq + intercept
def per_region_regression(table_stats, domain='cpu'):

    region_regression = {}

    for region in table_stats['app-config'].unique():
        region_inv_freq = 1.0 / table_stats[table_stats['app-config'] == region][f'{domain}-frequency']
        region_runtime = table_stats[table_stats['app-config'] == region]['runtime (s)']

        region_inv_freq = region_inv_freq.values.reshape(-1, 1)
        region_runtime = region_runtime.values.reshape(-1, 1)

        slope, intercept = calc_linear_regression(region_inv_freq, region_runtime)
        region_regression[region] = {"slope":slope, "intercept":intercept}
    return region_regression

def get_best_runtime_freq(region_regression, domain, region_name, perf_deg_factor, freq_range):

    freq_max = freq_range['max_freq']
    freq_min = freq_range['min_freq']
    # use the region regression to find min freq under perf deg allowance

    best_runtime = region_regression[region_name]["slope"] / freq_max + region_regression[region_name]["intercept"]

    runtime_freq = region_regression[region_name]["slope"] / (best_runtime*perf_deg_factor - region_regression[region_name]["intercept"])

    runtime_freq = min(max(runtime_freq, freq_min), freq_max)
    return (float)(runtime_freq), (float)(best_runtime*perf_deg_factor)

def get_energy_at_freq(table_stats, region, domain, freq):
    if domain == 'gpu':
        energy_col = 'gpu-energy (J)'
    else:
        energy_col = 'package-energy (J)'

    freq_subset = table_stats[table_stats['app-config'] == region]

    # Approximate by the nearest frequency
    freq = freq_subset.iloc[(freq_subset[f'{domain}-frequency'] - freq).abs().argmin()[f'{domain}-frequency']]
    freq_subset = freq_subset[freq_subset[f'{domain}-frequency'] == freq]
    if len(freq_subset[energy_col]) == 0:
        return None
    return min(freq_subset[energy_col])

def get_lowest_energy_freq(table_stats, domain, region, freq_perf, freq_range, freq_step=1e8):
    #If perf indicates max freq, don't need to do a search
    if freq_perf + freq_step >= freq_range['max_freq']:
        return freq_range['max_freq']

    if domain == 'gpu':
        energy_col = 'gpu-energy (J)'
    else:
        energy_col = 'package-energy (J)'

    freq_subset = table_stats.loc[(table_stats['app-config'] == region) & table_stats[f'{domain}-frequency'].between(freq_perf, freq_range['max_freq'])]

    row_idx = freq_subset[energy_col].argmin()

    return float(freq_subset[f'{domain}-frequency'].iloc[row_idx])

def main(output_name, data_file):
    freq_range={}
    region_regression={}
    table_stats = pd.read_hdf(data_file)
    #TODO: change to table_stats.dropna() and look for other such instances
    table_stats = table_stats[~table_stats['app-config'].isna()]

    domains = get_domains(table_stats)

    for domain in domains:
        freq_range[domain] = get_domain_freq_range(domain, table_stats)
        region_regression[domain] = per_region_regression(table_stats, domain)

    region_parameters = {}

    for domain in domains:
        params_out = open(f"{output_name}_{domain}.json", "w")
        region_parameters[domain] = {}
        for region_name in region_regression[domain]:
            freqs = []
            for phi in [i/10 for i in range(11)]:
                #Allowing perf degradation up to 25% (runtime increase by 1.25x)
                allowable_perf_deg = 1 + phi/4
                freq_r, _ = get_best_runtime_freq(region_regression[domain], domain, region_name, allowable_perf_deg, freq_range[domain]);
                freq  = get_lowest_energy_freq(table_stats, domain, region_name, freq_r, freq_range[domain])

                #TODO: Figure out a cleaner way to manage giant freq numbers
                freqs.append(freq/_FREQUENCY_NORM_FACTOR)

            region_parameters[domain][region_name] = freqs
        json.dump(region_parameters[domain], params_out)
        params_out.close()
        print(region_parameters)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output',
                        action='store',
                        default="region_parameters",
                        help='Prefix of the output json file(s)')
    parser.add_argument('--data-file',
                        action='store',
                        help='HDF containing stats data.')
    args = parser.parse_args()

    main(args.output, args.data_file)
