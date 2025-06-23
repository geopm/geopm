#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import json
import sys
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
        # Handle each package separately
        for package in table_stats['package'].unique():
            # Filter data for this specific package
            pkg_data = table_stats[(table_stats['app-config'] == region) & (table_stats['package'] == package)]

            # Skip if no data for this package
            if len(pkg_data) == 0:
                continue

            region_inv_freq = 1.0 / pkg_data[f'{domain}-frequency']
            region_runtime = pkg_data['runtime (s)']

            region_inv_freq = region_inv_freq.values.reshape(-1, 1)
            region_runtime = region_runtime.values.reshape(-1, 1)

            slope, intercept = calc_linear_regression(region_inv_freq, region_runtime)
            region_key = f"{region}_pkg{package}"
            region_regression[region_key] = {"slope":slope, "intercept":intercept, "package":package}
    return region_regression

def get_best_runtime_freq(region_regression, domain, region_name, perf_deg_factor, freq_range):

    freq_max = freq_range['max_freq']
    freq_min = freq_range['min_freq']
    # use the region regression to find min freq under perf deg allowance

    best_runtime = region_regression[region_name]["slope"] / freq_max + region_regression[region_name]["intercept"]

    runtime_freq = region_regression[region_name]["slope"] / (best_runtime*perf_deg_factor - region_regression[region_name]["intercept"])

    runtime_freq = min(max(runtime_freq, freq_min), freq_max)
    return (float)(runtime_freq), (float)(best_runtime*perf_deg_factor)

def get_energy_at_freq(table_stats, region, domain, freq, package=None):
    if domain == 'gpu':
        energy_col = 'gpu-energy (J)'
    else:
        energy_col = 'package-energy (J)'

    # Extract region name and package if this is a per-package region key
    if '_pkg' in region:
        region_name = region.split('_pkg')[0]
        package_num = int(region.split('_pkg')[1])
        freq_subset = table_stats[(table_stats['app-config'] == region_name) &
                                 (table_stats['package'] == package_num)]
    else:
        freq_subset = table_stats[table_stats['app-config'] == region]
        if package is not None:
            freq_subset = freq_subset[freq_subset['package'] == package]

    # Approximate by the nearest frequency
    if len(freq_subset) == 0:
        return None

    # Find index of row with frequency closest to target
    min_idx = (freq_subset[f'{domain}-frequency'] - freq).abs().idxmin()
    freq_subset = freq_subset.loc[min_idx:min_idx]

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

    # Extract region name and package if this is a per-package region key
    if '_pkg' in region:
        region_name = region.split('_pkg')[0]
        package_num = int(region.split('_pkg')[1])
        freq_subset = table_stats.loc[(table_stats['app-config'] == region_name) &
                                     (table_stats['package'] == package_num) &
                                     table_stats[f'{domain}-frequency'].between(freq_perf, freq_range['max_freq'])]
    else:
        freq_subset = table_stats.loc[(table_stats['app-config'] == region) &
                                     table_stats[f'{domain}-frequency'].between(freq_perf, freq_range['max_freq'])]

    if len(freq_subset) == 0:
        return freq_range['max_freq']  # Return max freq if no data found

    row_idx = freq_subset[energy_col].idxmin()

    return float(freq_subset.loc[row_idx, f'{domain}-frequency'])

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

    # Group regions by their base name (without package suffix)
    region_packages = {}
    for domain in domains:
        region_packages[domain] = {}
        for region_key in region_regression[domain]:
            base_region = region_key.split('_pkg')[0] if '_pkg' in region_key else region_key
            pkg = region_regression[domain][region_key].get('package', 0)

            if base_region not in region_packages[domain]:
                region_packages[domain][base_region] = []
            region_packages[domain][base_region].append(pkg)

    # Process each domain
    for domain in domains:
        # Create a separate output file for each package
        packages = sorted(list(set([region_regression[domain][r].get('package', 0)
                                   for r in region_regression[domain]])))

        for pkg in packages:
            region_parameters = {}
            params_out = open(f"{output_name}_{domain}_pkg{pkg}.json", "w")

            # Process each region for this package
            for base_region in region_packages[domain]:
                if pkg not in region_packages[domain][base_region]:
                    continue

                region_key = f"{base_region}_pkg{pkg}"
                # Skip if this specific package-region combination doesn't exist
                if region_key not in region_regression[domain]:
                    continue

                freqs = []
                for phi in [i/10 for i in range(11)]:
                    #Allowing perf degradation up to 25% (runtime increase by 1.25x)
                    allowable_perf_deg = 1 + phi/4
                    freq_r, _ = get_best_runtime_freq(region_regression[domain], domain, region_key,
                                                    allowable_perf_deg, freq_range[domain])
                    freq = get_lowest_energy_freq(table_stats, domain, region_key,
                                                freq_r, freq_range[domain])

                    #TODO: Figure out a cleaner way to manage giant freq numbers
                    freqs.append(freq/_FREQUENCY_NORM_FACTOR)

                # Store in the output using the base region name (without package suffix)
                region_parameters[base_region] = freqs

            json.dump(region_parameters, params_out)
            params_out.close()
            print(f"Generated parameters for domain: {domain}, package: {pkg}")

    # For backward compatibility, also generate combined files
    for domain in domains:
        combined_params = {}
        params_out = open(f"{output_name}_{domain}.json", "w")

        # For each base region, average the frequencies across packages
        for base_region in region_packages[domain]:
            all_freqs = []
            for pkg in region_packages[domain][base_region]:
                region_key = f"{base_region}_pkg{pkg}"
                if region_key in region_regression[domain]:
                    freqs = []
                    for phi in [i/10 for i in range(11)]:
                        allowable_perf_deg = 1 + phi/4
                        freq_r, _ = get_best_runtime_freq(region_regression[domain], domain, region_key,
                                                        allowable_perf_deg, freq_range[domain])
                        freq = get_lowest_energy_freq(table_stats, domain, region_key,
                                                    freq_r, freq_range[domain])
                        freqs.append(freq/_FREQUENCY_NORM_FACTOR)
                    all_freqs.append(freqs)

            # Average the frequencies if we have data for multiple packages
            if all_freqs:
                combined_params[base_region] = [sum(x)/len(x) for x in zip(*all_freqs)]

        json.dump(combined_params, params_out)
        params_out.close()
        print(f"Generated combined parameters for domain: {domain}")

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
