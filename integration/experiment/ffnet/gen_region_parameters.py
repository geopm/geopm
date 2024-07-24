#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import json
import pandas as pd
from sklearn import datasets, linear_model
import argparse

def get_domains(table_stats):
    domains = []
    for domain in ['cpu', 'gpu', 'uncore']:
        if f'{domain}-frequency' in table_stats.columns:
            domains.append(domain)
    return domains

def get_domain_freq_range(domain, table_stats):
    if f'{domain}-frequency' in table_stats:
        return {'min_freq' : min(table_stats[f'{domain}-frequency']),
                'max_freq' : max(table_stats[f'{domain}-frequency'])}

#Outputs runtime = slope * inv_freq + intercept
def per_region_regression(table_stats, domain='cpu'):

    regr = linear_model.LinearRegression()
    region_regression = {}

    for region in table_stats['app-config'].unique():
        region_inv_freq = 1.0 / table_stats[table_stats['app-config'] == region][f'{domain}-frequency']
        region_runtime = table_stats[table_stats['app-config'] == region]['runtime (s)']

        region_inv_freq = region_inv_freq.values.reshape(-1, 1)
        region_runtime = region_runtime.values.reshape(-1, 1)

        regr.fit(region_inv_freq, region_runtime)

        region_regression[region] = {"slope":regr.coef_, "intercept":regr.intercept_}

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
    freq = freq_subset.iloc[(freq_subset[f'{domain}-frequency']-freq).abs().argsort()[:1]][f'{domain}-frequency'].tolist()[0]

    freq_subset = freq_subset[freq_subset[f'{domain}-frequency'] == freq]
    if len(freq_subset[energy_col]) == 0:
        return None
    return min(freq_subset[energy_col])

def get_lowest_energy_freq(table_stats, domain, region, freq_r, freq_range):
    #If perf indicates max freq, don't need to do a search
    if freq_r + 1e8 >= freq_range['max_freq']:
        return freq_range['max_freq']

    if domain == 'gpu':
        energy_col = 'gpu-energy (J)'
    else:
        energy_col = 'package-energy (J)'

    freq_subset = table_stats[table_stats['app-config'] == region]
    freq_subset = freq_subset[freq_subset[f'{domain}-frequency'] >= freq_r]
    freq_subset = freq_subset[freq_subset[f'{domain}-frequency'] <= freq_range['max_freq']]

    row_idx = freq_subset[energy_col].argmin()

    return (float)(freq_subset[f'{domain}-frequency'].iloc[row_idx])

def main(output_name, data_file, region_ignore=None):

    #Regions to ignore for training
    if region_ignore == None:
        region_list = []
    else:
        region_list = region_ignore.split(",")
    region_ignore = ['NAN'] + region_list

    freq_range={}
    region_regression={}
    table_stats = pd.read_hdf(data_file)
    table_stats = table_stats[~table_stats['app-config'].isna()]
    table_stats = table_stats[~table_stats['app-config'].isin(region_ignore)]

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

                freqs.append(freq)

            region_parameters[domain][region_name] = freqs
        json.dump(region_parameters[domain], params_out)
        params_out.close()
        print(region_parameters)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    #common_args.add_output_dir(parser)
    parser.add_argument('--output',
                        action='store',
                        default="region_parameters",
                        help='Prefix of the output json file(s)')
    parser.add_argument('--data-file',
                        action='store',
                        help='HDF containing stats data.')
    parser.add_argument('--ignore',
                        help='Comma-separated hashes of any regions to ignore.',
                        dest="region_ignore",
                        default=None)
    args = parser.parse_args()

    main(args.output, args.data_file, args.region_ignore)
