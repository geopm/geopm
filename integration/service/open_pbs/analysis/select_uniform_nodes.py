#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""Select N nodes whose FOM values are evenly distributed (quantile-based)
at a given board power limit.

Outputs one hostname per line to stdout.
"""

import argparse
import code
import glob
import hashlib
import json
import os
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from yaml import load
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader


def parse_args():
    parser = argparse.ArgumentParser(
        description='Select nodes with evenly distributed FOM at a given power budget.')
    parser.add_argument('node_count', type=int,
                        help='Number of nodes to select.')
    parser.add_argument('board_power_limit', type=int,
                        help='Board power limit (watts) to evaluate. '
                             'Must exactly match a measured power level.')
    parser.add_argument('--report-dirs', nargs='+',
                        help='Directories containing GEOPM report files.')
    parser.add_argument('--cache', nargs='?', const='.compare_cache', default=None,
                        help='Load data from pre-built HDF5 cache files '
                             '(cache_*.h5). Optionally accepts a path to the '
                             'cache directory (default: .compare_cache).')
    parser.add_argument('--filter-hosts', metavar='FILE', default=None,
                        help='Path to a file containing hostnames (one per line) '
                             'to exclude from the dataset.')
    parser.add_argument('--outliers', nargs='+', default=None,
                        metavar='POWER,OP,THRESH',
                        help='FOM outlier rules. Each rule is '
                             '"POWER,OPERATOR,THRESHOLD" where OPERATOR is '
                             '"lt" or "gt". Any host with a trial violating a '
                             'rule is removed from the dataset.')
    parser.add_argument('--use-region',
                        help='Use time spent in the given region instead of '
                             'Application Totals.')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print additional diagnostic information to stderr.')
    parser.add_argument('--interactive', action='store_true',
                        help='Drop into an interactive Python session after '
                             'loading and filtering data.')
    return parser.parse_args()


def load_host_filter(path):
    """Read a hostname filter file and return a set of hostnames to exclude."""
    hosts = set()
    with open(path) as f:
        for raw in f:
            line = raw.split('#', 1)[0].strip()
            if line:
                hosts.add(line)
    return hosts


def apply_outlier_filter(df, rules, verbose=False):
    """Remove hosts whose FOM violates any outlier rule.

    Each rule is "POWER,OPERATOR,THRESHOLD" where OPERATOR is lt or gt.
    A single bad trial flags the entire host for removal.
    """
    col = 'BOARD_POWER_LIMIT_CONTROL'
    metric = 'FOM'
    for required in ('host', col, metric):
        if required not in df.columns:
            raise KeyError(
                f"Column '{required}' not found in DataFrame. "
                f"Available columns: {list(df.columns)}")

    outlier_hosts = set()
    for rule_str in rules:
        parts = rule_str.split(',')
        if len(parts) != 3:
            raise ValueError(
                f"Outlier rule must be 'POWER,OPERATOR,THRESHOLD', got: {rule_str}")
        power = int(parts[0].strip())
        op = parts[1].strip().lower()
        threshold = float(parts[2].strip())
        if op not in ('lt', 'gt'):
            raise ValueError(f"Operator must be 'lt' or 'gt', got: {op}")

        subset = df[df[col].astype(int) == power]
        if subset.empty:
            print(f'Outlier rule {rule_str}: no data at {power} W — skipped',
                  file=sys.stderr)
            continue

        if op == 'lt':
            flagged = subset[subset[metric] < threshold]
            desc = f'FOM < {threshold:g}'
        else:
            flagged = subset[subset[metric] > threshold]
            desc = f'FOM > {threshold:g}'

        hosts_in_rule = sorted(flagged['host'].unique())
        if hosts_in_rule:
            outlier_hosts.update(hosts_in_rule)
            print(f'Outlier rule {power} W {desc}: {len(flagged)} trial(s) '
                  f'from {len(hosts_in_rule)} host(s): {hosts_in_rule}',
                  file=sys.stderr)
        elif verbose:
            print(f'Outlier rule {power} W {desc}: no outlier trials found',
                  file=sys.stderr)

    if outlier_hosts:
        before = len(df)
        df = df[~df['host'].isin(outlier_hosts)].reset_index(drop=True)
        print(f'Removed {len(outlier_hosts)} outlier host(s) '
              f'({before - len(df)} rows) from dataset: '
              f'{sorted(outlier_hosts)}', file=sys.stderr)

    return df


def validate_dataset(df, min_trials=5, verbose=False):
    """Filter out hosts with incomplete or insufficient data.

    Removes hosts that:
    1. Do not have data for every BOARD_POWER_LIMIT_CONTROL value in the dataset.
    2. Have missing FOM at any power level.
    3. Have fewer than *min_trials* trials at any power level.

    Returns the filtered DataFrame.
    """
    col = 'BOARD_POWER_LIMIT_CONTROL'
    if 'host' not in df.columns or col not in df.columns:
        return df

    hosts_to_drop = set()
    all_limits = set(df[col].dropna().unique())

    for host, hdf in df.groupby('host'):
        host_limits = set(hdf[col].dropna().unique())

        # 1. Missing power budgets
        if host_limits != all_limits:
            missing = sorted(all_limits - host_limits)
            if verbose:
                print(f'Filtering {host}: missing power budgets {missing}',
                      file=sys.stderr)
            hosts_to_drop.add(host)
            continue

        # 2. Missing FOM at any power level
        if 'FOM' in hdf.columns:
            fom_by_limit = hdf.groupby(col)['FOM'].apply(
                lambda s: s.notna().any()
            )
            missing_fom = sorted(
                fom_by_limit.index[~fom_by_limit].tolist()
            )
            if missing_fom:
                if verbose:
                    print(f'Filtering {host}: missing FOM at power levels '
                          f'{missing_fom}', file=sys.stderr)
                hosts_to_drop.add(host)
                continue

        # 3. Fewer than min_trials at any power level
        trials_per_limit = hdf.groupby(col).size()
        under = trials_per_limit[trials_per_limit < min_trials]
        if not under.empty:
            if verbose:
                detail = {int(k): int(v) for k, v in under.items()}
                print(f'Filtering {host}: fewer than {min_trials} trials '
                      f'at power levels {detail}', file=sys.stderr)
            hosts_to_drop.add(host)
            continue

    if hosts_to_drop:
        before_hosts = df['host'].nunique()
        before_rows = len(df)
        df = df[~df['host'].isin(hosts_to_drop)].reset_index(drop=True)
        print(f'validate_dataset: removed {len(hosts_to_drop)} host(s) '
              f'({before_rows - len(df)} rows), '
              f'{df["host"].nunique()}/{before_hosts} hosts remain',
              file=sys.stderr)
    elif verbose:
        print(f'validate_dataset: all {df["host"].nunique()} hosts passed '
              f'validation', file=sys.stderr)

    return df


def load_cached_data(cache_path, verbose=False):
    """Load pre-built HDF5 caches (cache_*.h5)."""
    root = Path(cache_path).expanduser().resolve()
    h5_files = sorted(root.rglob('cache_*.h5'))
    if not h5_files:
        raise FileNotFoundError(
            f'No cache_*.h5 files found under {root}')

    frames = []
    for h5 in h5_files:
        for key in ('app_report', 'reports'):
            try:
                app_df = pd.read_hdf(h5, key=key)
                frames.append(app_df)
                break
            except KeyError:
                continue
        else:
            if verbose:
                print(f'Warning: No app_report or reports key in {h5}',
                      file=sys.stderr)

    if not frames:
        raise RuntimeError(
            f'No usable data found in cache files under {root}')

    df = pd.concat(frames, ignore_index=True)

    # Normalise column names
    rename_map = {}
    if 'Profile' in df.columns and 'profile' not in df.columns:
        rename_map['Profile'] = 'profile'
    if 'Agent' in df.columns and 'agent' not in df.columns:
        rename_map['Agent'] = 'agent'
    if rename_map:
        df.rename(columns=rename_map, inplace=True)

    if verbose:
        print(f'Loaded {len(df)} rows from {len(h5_files)} cache file(s) '
              f'under {root}', file=sys.stderr)
    return df


def load_report_dirs(report_dirs, use_region=None, verbose=False):
    """Load data from GEOPM YAML report files."""
    report_paths = []
    for report_path in report_dirs:
        if glob.has_magic(report_path):
            for match_path in glob.glob(report_path):
                if os.path.isdir(match_path):
                    for root, _, files in os.walk(match_path):
                        report_paths.extend(
                            os.path.join(root, filename)
                            for filename in files
                            if 'report' in filename)
        elif os.path.isdir(report_path):
            for root, _, files in os.walk(report_path):
                report_paths.extend(
                    os.path.join(root, filename)
                    for filename in files
                    if 'report' in filename)

    report_paths = sorted(set(report_paths))
    if not report_paths:
        raise FileNotFoundError('No report files found in specified directories.')

    # Check for cached version
    report_hash_inputs = {
        'report_dirs': report_paths,
        'use_fom': True,
        'use_region': use_region,
    }
    report_hash = hashlib.sha256(
        json.dumps(report_hash_inputs, sort_keys=True).encode('utf-8')
    ).hexdigest()
    hdf5_path = f'{report_hash}.h5'

    try:
        df = pd.read_hdf(hdf5_path, key='reports')
    except (FileNotFoundError, KeyError, OSError):
        data_list = []
        for rp in report_paths:
            with open(rp) as f:
                report = load(f, Loader=SafeLoader)
            if report is None:
                if verbose:
                    print(f'Warning: Skipping empty report {rp}',
                          file=sys.stderr)
                continue
            if 'Figure of Merit' not in report:
                if verbose:
                    print(f'Warning: Skipping report without FOM: {rp}',
                          file=sys.stderr)
                continue
            for host, host_data in report['Hosts'].items():
                if use_region is not None:
                    try:
                        report_data = next(
                            r for r in host_data['Regions']
                            if r['region'] == use_region)
                    except StopIteration:
                        print(f'Error: report {rp} does not contain region '
                              f'{use_region}', file=sys.stderr)
                        sys.exit(1)
                else:
                    report_data = host_data['Application Totals']
                report_data['report_path'] = rp
                report_data['host'] = host
                report_data['agent'] = report['Agent']
                report_data['profile'] = report['Profile']
                report_data['FOM'] = report['Figure of Merit']
                data_list.append(report_data)

        if not data_list:
            raise RuntimeError('No usable data found in report files.')
        df = pd.DataFrame(data_list)
        df.to_hdf(hdf5_path, key='reports', mode='w')

    if verbose:
        print(f'Loaded {len(df)} rows from {len(report_paths)} report file(s)',
              file=sys.stderr)
    return df


def select_uniform_nodes(df, board_power_limit, node_count, verbose=False):
    """Select *node_count* nodes with quantile-spaced FOM at *board_power_limit*.

    Returns a list of hostnames.
    """
    col = 'BOARD_POWER_LIMIT_CONTROL'

    # Filter to the exact power level
    at_power = df[df[col].astype(int) == board_power_limit]
    if at_power.empty:
        available = sorted(df[col].astype(int).unique())
        raise ValueError(
            f'No data at board power limit {board_power_limit} W. '
            f'Available levels: {available}')

    # Compute per-host mean FOM at this power level
    host_fom = at_power.groupby('host')['FOM'].mean().sort_values()

    available_hosts = len(host_fom)
    if node_count > available_hosts:
        print(f'WARNING: requested {node_count} nodes but only '
              f'{available_hosts} available after filtering. '
              f'Returning all {available_hosts} nodes.', file=sys.stderr)
        return list(host_fom.index)

    if node_count <= 0:
        raise ValueError('node_count must be positive.')

    if node_count == 1:
        # Return the median host
        median_idx = len(host_fom) // 2
        return [host_fom.index[median_idx]]

    # Quantile-based selection: pick one host per quantile bin.
    # Generate N target quantile positions evenly spaced from 0 to 1.
    quantiles = np.linspace(0, 1, node_count)
    target_foms = np.quantile(host_fom.values, quantiles)

    selected = []
    remaining_hosts = host_fom.copy()
    for target in target_foms:
        # Find the host closest to the target FOM that hasn't been selected
        distances = (remaining_hosts - target).abs()
        best_host = distances.idxmin()
        selected.append(best_host)
        remaining_hosts = remaining_hosts.drop(best_host)

    if verbose:
        print(f'Selected {len(selected)} nodes at {board_power_limit} W:',
              file=sys.stderr)
        for hostname in selected:
            print(f'  {hostname}: FOM={host_fom[hostname]:.6g}',
                  file=sys.stderr)
        print(f'FOM range: [{host_fom.min():.6g}, {host_fom.max():.6g}]',
              file=sys.stderr)

    return selected


def main():
    args = parse_args()

    # Load data
    if args.cache is not None:
        df = load_cached_data(args.cache, verbose=args.verbose)
    elif args.report_dirs is not None:
        df = load_report_dirs(args.report_dirs, use_region=args.use_region,
                              verbose=args.verbose)
    else:
        print('ERROR: Either --cache or --report-dirs must be specified.',
              file=sys.stderr)
        sys.exit(1)

    # Require FOM column
    if 'FOM' not in df.columns:
        print('ERROR: FOM column not found in dataset. This script requires '
              'Figure of Merit data.', file=sys.stderr)
        sys.exit(1)

    # Apply hostname filter
    if args.filter_hosts:
        excluded_hosts = load_host_filter(args.filter_hosts)
        if not excluded_hosts:
            print(f'WARNING: --filter-hosts file {args.filter_hosts} contained '
                  f'no hostnames.', file=sys.stderr)
        elif 'host' not in df.columns:
            print('WARNING: --filter-hosts specified but DataFrame has no '
                  '"host" column; skipping.', file=sys.stderr)
        else:
            present = set(df['host'].unique())
            matched = excluded_hosts & present
            if matched:
                before = len(df)
                df = df[~df['host'].isin(matched)].reset_index(drop=True)
                if args.verbose:
                    print(f'Filtered {len(matched)} host(s) from '
                          f'{args.filter_hosts} ({before - len(df)} rows '
                          f'removed)', file=sys.stderr)

    # Apply outlier filtering
    if args.outliers:
        df = apply_outlier_filter(df, args.outliers, verbose=args.verbose)

    # Validate dataset (remove hosts with incomplete sweeps or missing FOM)
    df = validate_dataset(df, verbose=args.verbose)

    if args.interactive:
        print('Dropping into interactive session. DataFrame is available as "df".',
              file=sys.stderr)
        code.interact(local=dict(globals(), **locals()))

    # Select nodes
    selected = select_uniform_nodes(df, args.board_power_limit,
                                    args.node_count, verbose=args.verbose)

    # Output one hostname per line (sorted)
    for hostname in sorted(selected):
        print(hostname)


if __name__ == '__main__':
    main()
