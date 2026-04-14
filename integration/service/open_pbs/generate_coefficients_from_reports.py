#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import argparse
import pandas as pd
import numpy as np
import sklearn.metrics
from scipy import optimize
import json
import sys
import hashlib
import os
import glob
from pathlib import Path
from yaml import load
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

parser = argparse.ArgumentParser('Generate a summary of an endpoint experiment')
parser.add_argument('max_power', type=float,
                    help='Maximum power limit allowed, in watts.')
parser.add_argument('--per-host', action='store_true',
                    help='Generate a set of coefficients for each host in a report profile group.')
parser.add_argument('--show-min-max-range', action='store_true',
                    help='Show the source data min-max range as shaded regions')
parser.add_argument('--show-samples', action='store_true',
                    help='Show the source data samples as scatterplot points')
parser.add_argument('--show-legend', action='store_true',
                    help='Show the legend in the plot')
parser.add_argument('--report-dirs', nargs='+')
parser.add_argument('-v', '--verbose', action='store_true',
                    help='Print additional information about coefficient selection.')
parser.add_argument('--plot-path',
                    help='Path to save a plot of the power-performance data. Default: no plots are generated')
parser.add_argument('--cache', nargs='?', const='.compare_cache', default=None,
                    help='Load data directly from pre-built HDF5 cache files '
                         '(cache_*.h5 from plot.py), skipping report parsing. '
                         'Optionally accepts a path to the cache directory '
                         '(default: .compare_cache).')
parser.add_argument('--outliers', nargs='+', default=None,
                    metavar='POWER,OP,THRESH',
                    help='FOM outlier rules.  Each rule is '
                         '"POWER,OPERATOR,THRESHOLD" where OPERATOR is '
                         '"lt" or "gt".  Any host with a trial '
                         'violating a rule is removed from the dataset.  '
                         'Example: --outliers "3200,lt,4e6" "3200,gt,4.7e6"')

group = parser.add_mutually_exclusive_group()
group.add_argument('--use-region', help='Use time spent in the given region.')
group.add_argument('--use-fom', action='store_true', help='Use the Figure of Merit')
parser.add_argument('--model-type', default='original-quadratic',
                    choices=['original-quadratic', 'piecewise-linear'],
                    help='Type of performance model to generate. '
                         'original-quadratic fits A*(x0-x)^2+B*(x0-x)+C. '
                         'piecewise-linear stores average FOM at each '
                         'measured power level. (default: original-quadratic)')

args = parser.parse_args()


def get_coefficients(df):
    """Given a DataFrame containing power and performance data, return a tuple
    of model coefficients that fit to the data.

    Returns: x0, A, B, C
    """
    columns = ['agent', 'profile', 'host', 'slowdown', 'BOARD_POWER_LIMIT_CONTROL', 'runtime (s)', 'BOARD_ENERGY']
    if not args.use_fom:
        df = df[columns]
    else:
        df = df[columns + ['FOM']]

    X = np.reshape(df['BOARD_POWER_LIMIT_CONTROL'].values, (-1, 1)).astype('float64') / args.max_power
    y = df['slowdown'].values

    res = optimize.minimize(
        loss,
        # Initial guess: x0=1 (max power), everything else is zero (i.e., a flat line)
        [1, 0, 0, 0],
        args=(y, X),
        jac=loss_jac,
        # y = A * (x0 - x)**2 + B * (x0 - x) + C
        # dy/dx: -A*2*x0 + A*2*x - B
        constraints=(
            dict(type='ineq', fun=lambda x: 2 * x[1]), # y''(x) >= 0, Slowdown decreases as power increases in the lower power domain
            dict(type='ineq', fun=lambda x: 2 * x[1] * (x[0] - 1) + x[2]), # y'(1) <= 0, Slowdown is not increasing at Pmax
            dict(type='ineq', fun=lambda x: x[1] * (x[0] - 1) ** 2 + x[2] * (x[0] - 1) + x[3]), # y(1) >= 0, Slowdown not better than best known at Pmax
        )
    )
    params = res.x
    if not res.success:
        print(res.message, file=sys.stderr)

    if args.verbose:
        y_train_pred = slowdown_at_power(X, *params)
        r2score = sklearn.metrics.r2_score(y, y_train_pred)
        print(f'{host} training R2 Score: {r2score}', file=sys.stderr)

    return params


def slowdown_at_power(power, x0, A, B, C):
    """Return the slowdown factor (1 == 100% slowdown) given power normalized
    to max power at power=1.
    """
    return A * (x0 - power.flatten())**2 + B * (x0 - power.flatten()) + C


def power_at_slowdown(slowdown, x0, A, B, C):
    """Return power as a fraction of max power give the slowdown factor.
    """
    return x0 - (-B + np.sqrt(B**2 - 4 * A * (C - slowdown))) / (2 * A)


def loss(params, slowdown, power):
    """Sum of square differences loss function to fit slowdown_at_power(power)
    against a ground truth slowdown.
    """
    return np.sum((slowdown - slowdown_at_power(power, *params))**2)


def loss_jac(params, slowdown, power):
    """Return the gradient of loss(slowdown, power) with respect to params.
    """
    J = np.empty(params.size)
    x0, A, B, C = params
    P = power.flatten()
    neg_two_resid = -2*(slowdown - slowdown_at_power(power, *params))
    x0mP = x0 - P
    J[0] = np.sum(neg_two_resid*(2*A*x0mP + B))
    J[1] = np.sum(neg_two_resid*(x0mP**2))
    J[2] = np.sum(neg_two_resid*x0mP)
    J[3] = np.sum(neg_two_resid)
    return J


def apply_outlier_filter(df, rules):
    """Remove hosts whose FOM violates any outlier rule.

    Each *rule* is a string ``"POWER,OPERATOR,THRESHOLD"`` where
    OPERATOR is ``lt`` or ``gt``.  A single bad trial flags the entire
    host for removal.

    Returns the filtered DataFrame.
    """
    col = 'BOARD_POWER_LIMIT_CONTROL'
    metric = 'FOM'
    for required in ('host', col, metric):
        if required not in df.columns:
            raise KeyError(
                f"Column '{required}' not found in DataFrame. "
                f"Available columns: {list(df.columns)}"
            )

    outlier_hosts = set()
    for rule_str in rules:
        parts = rule_str.split(',')
        if len(parts) != 3:
            raise ValueError(
                f"Outlier rule must be 'POWER,OPERATOR,THRESHOLD', got: {rule_str}"
            )
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
        elif args.verbose:
            print(f'Outlier rule {power} W {desc}: no outlier trials found',
                  file=sys.stderr)

    if outlier_hosts:
        before = len(df)
        df = df[~df['host'].isin(outlier_hosts)].reset_index(drop=True)
        print(f'Removed {len(outlier_hosts)} outlier host(s) '
              f'({before - len(df)} rows) from dataset: '
              f'{sorted(outlier_hosts)}', file=sys.stderr)

    return df


def validate_dataset(df, min_trials=5):
    """Filter out hosts with incomplete or insufficient data.

    Removes hosts that:
    1. Do not have data for every BOARD_POWER_LIMIT_CONTROL value in the dataset.
    2. Have missing FOM at any power level (when --use-fom is set).
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
            print(f'Filtering {host}: missing power budgets {missing}',
                  file=sys.stderr)
            hosts_to_drop.add(host)
            continue

        # 2. Missing FOM at any power level
        if args.use_fom and 'FOM' in hdf.columns:
            fom_by_limit = hdf.groupby(col)['FOM'].apply(
                lambda s: s.notna().any()
            )
            missing_fom = sorted(
                fom_by_limit.index[~fom_by_limit].tolist()
            )
            if missing_fom:
                print(f'Filtering {host}: missing FOM at power levels '
                      f'{missing_fom}', file=sys.stderr)
                hosts_to_drop.add(host)
                continue

        # 3. Fewer than min_trials at any power level
        trials_per_limit = hdf.groupby(col).size()
        under = trials_per_limit[trials_per_limit < min_trials]
        if not under.empty:
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
    else:
        print(f'validate_dataset: all {df["host"].nunique()} hosts passed '
              f'validation', file=sys.stderr)

    return df


def load_cached_data(cache_path):
    """Load pre-built HDF5 caches (cache_*.h5), skipping report parsing.

    Reads every ``cache_*.h5`` file under *cache_path* (recursively) and
    concatenates the ``app_report`` DataFrames.  Column names are
    normalised to match what the rest of this script expects.
    """
    root = Path(cache_path).expanduser().resolve()
    h5_files = sorted(root.rglob('cache_*.h5'))
    if not h5_files:
        raise FileNotFoundError(
            f'No cache_*.h5 files found under {root}'
        )

    frames = []
    for h5 in h5_files:
        loaded = False
        # Try io.py / RawReportCollection key first, then the hash-cache key
        for key in ('app_report', 'reports'):
            try:
                app_df = pd.read_hdf(h5, key=key)
                frames.append(app_df)
                loaded = True
                break
            except KeyError:
                continue
        if not loaded and args.verbose:
            print(f'Warning: No app_report or reports key in {h5}',
                  file=sys.stderr)

    if not frames:
        raise RuntimeError(
            f'No usable data found in cache files under {root}'
        )

    df = pd.concat(frames, ignore_index=True)

    # --- Normalise column names to match what the script expects ----------
    rename_map = {}
    if 'Profile' in df.columns and 'profile' not in df.columns:
        rename_map['Profile'] = 'profile'
    if 'Agent' in df.columns and 'agent' not in df.columns:
        rename_map['Agent'] = 'agent'
    if rename_map:
        df.rename(columns=rename_map, inplace=True)

    # --- Derive job_host_count (unique hosts per report file) -------------
    if 'job_host_count' not in df.columns:
        if 'report_file' in df.columns and 'host' in df.columns:
            jhc = df.groupby('report_file')['host'].transform('nunique')
            df['job_host_count'] = jhc
        elif 'host' in df.columns:
            # Fallback: count all unique hosts
            df['job_host_count'] = df['host'].nunique()
        else:
            df['job_host_count'] = 1

    # --- Derive slowdown metric -------------------------------------------
    if 'slowdown metric' not in df.columns:
        if args.use_fom and 'FOM' in df.columns:
            df['slowdown metric'] = 1.0 / df['FOM']
        elif 'runtime (s)' in df.columns:
            df['slowdown metric'] = df['runtime (s)']
        else:
            raise KeyError(
                'Cannot compute slowdown metric: need FOM or runtime (s)'
            )

    print(f'Loaded {len(df)} rows from {len(h5_files)} cache file(s) '
          f'under {root}', file=sys.stderr)
    return df


data_list = list()
report_paths = list()
if args.cache is not None:
    df = load_cached_data(args.cache)
elif args.report_dirs is not None:
    for report_path in args.report_dirs:
        if glob.has_magic(report_path):
            for match_path in glob.glob(report_path):
                if os.path.isdir(match_path):
                    for root, _, files in os.walk(match_path):
                        report_paths.extend(
                            os.path.join(root, filename)
                            for filename in files
                            if 'report' in filename
                        )
        elif os.path.isdir(report_path):
            for root, _, files in os.walk(report_path):
                report_paths.extend(
                    os.path.join(root, filename)
                    for filename in files
                    if 'report' in filename
                )

    report_paths = sorted(set(report_paths))
    report_hash_inputs = {
        'report_dirs': report_paths,
        'use_fom': bool(args.use_fom),
        'use_region': args.use_region,
    }
    report_hash = hashlib.sha256(json.dumps(report_hash_inputs, sort_keys=True).encode('utf-8')).hexdigest()
    hdf5_path = f'{report_hash}.h5'

    try:
        df = pd.read_hdf(hdf5_path, key='reports')
    except (FileNotFoundError, KeyError, OSError):
        for report_path in report_paths:
            with open(report_path) as f:
                report = load(f, Loader=SafeLoader)
            if report is None:
                if args.verbose:
                    print(f'Warning: Skipping empty report {report_path}', file=sys.stderr)
                continue
            if args.use_fom and 'Figure of Merit' not in report:
                if args.verbose:
                    print(f'Warning: Skipping report since --use-fom was specified and report is missing Figure of Merit: {report_path}', file=sys.stderr)
                continue
            job_host_count = len(report['Hosts'])
            for host, host_data in report['Hosts'].items():
                if args.use_region is not None:
                    try:
                        report_data = next(r for r in host_data['Regions'] if r['region'] == args.use_region)
                    except StopIteration:
                        print(f'Error: report {report_path} does not contain region {args.use_region}', file=sys.stderr)
                        sys.exit(1)
                else:
                    report_data = host_data['Application Totals']
                report_data['report_path'] = report_path
                report_data['host'] = host
                report_data['job_host_count'] = job_host_count
                report_data['agent'] = report['Agent']
                report_data['profile'] = report['Profile']
                if 'Figure of Merit' in report:
                    report_data['FOM'] = report['Figure of Merit']

                report_data['slowdown metric'] = (1 / report_data['FOM']) if (args.use_fom and 'Figure of Merit' in report) else report_data['runtime (s)']
                data_list.append(report_data)

        df = pd.DataFrame(data_list)
        df.to_hdf(hdf5_path, key='reports', mode='w')
else:
    parser.error('Either --cache or --report-dirs must be specified.')

# --- Outlier filtering (runs before normalization) ------------------------
if args.outliers:
    if 'FOM' not in df.columns:
        print('WARNING: --outliers requires FOM column; skipping outlier '
              'filtering.', file=sys.stderr)
    else:
        df = apply_outlier_filter(df, args.outliers)

# --- Data validation (runs after outlier filtering, before normalization) --
df = validate_dataset(df)

host = df['host'].iloc[-1] if 'host' in df.columns and not df['host'].empty else 'all'

if 'BOARD_ENERGY' not in df.columns:
    print('Board energy is absent from reports. Summing package, gpu, and dram energy instead.', file=sys.stderr)
    df['BOARD_ENERGY'] = df['package-energy (J)'] + df['gpu-energy (J)'] + df['dram-energy (J)']

# Get the reference performance per (profile,host count) pair
profile_reference_slowdown = df.groupby(['profile', 'job_host_count']).apply(
    lambda x: x.loc[x['BOARD_POWER_LIMIT_CONTROL'] == x['BOARD_POWER_LIMIT_CONTROL'].max(), 'slowdown metric'].mean())
df['slowdown'] = df['slowdown metric'] / pd.MultiIndex.from_frame(df[['profile', 'job_host_count']]).map(profile_reference_slowdown) - 1

output = dict(
    max_power=args.max_power,
    profiles=dict())
for profile, df_profile in df.groupby('profile'):
    if args.model_type == 'piecewise-linear':
        if not args.use_fom or 'FOM' not in df_profile.columns:
            print('ERROR: --model-type piecewise-linear requires --use-fom and '
                  'FOM data in the dataset.', file=sys.stderr)
            sys.exit(1)
        col = 'BOARD_POWER_LIMIT_CONTROL'
        metric = 'FOM'
        profile_entry = dict(model_type='piecewise-linear')
        if args.per_host:
            profile_entry['hosts'] = dict()
            for host, df_host in df_profile.groupby('host'):
                host_avg = df_host.groupby(col)[metric].mean()
                profile_entry['hosts'][host] = dict(
                    model={str(int(p)): float(v)
                           for p, v in host_avg.items()})
        else:
            # Profile-level aggregate model: average FOM across all hosts per power level
            profile_avg = df_profile.groupby(col)[metric].mean()
            profile_entry['model'] = {str(int(p)): float(v)
                                      for p, v in profile_avg.items()}
        output['profiles'][profile] = profile_entry
    else:
        # original-quadratic
        profile_entry = dict(model_type='original-quadratic')
        if args.per_host:
            profile_entry['hosts'] = dict()
            for host, df_host in df_profile.groupby('host'):
                profile_entry['hosts'][host] = dict(
                    model=dict(zip(('x0', 'A', 'B', 'C'), get_coefficients(df_host))))
        else:
            profile_entry['model'] = dict(
                zip(('x0', 'A', 'B', 'C'), get_coefficients(df_profile)))
        output['profiles'][profile] = profile_entry

if args.per_host:
    # If multiple profiles are included here, default to using the first one.
    output['node_profile_name'] = next(iter(output['profiles']))

# Optionally plot the models with or without the fit samples
if args.plot_path is not None:
    import matplotlib.pyplot as plt
    import matplotlib.ticker as mtick
    with plt.style.context('seaborn-darkgrid'):
        fig, ax = plt.subplots(figsize=(4, 3))
        min_control = df.loc[df['BOARD_POWER_LIMIT_CONTROL'] != 0, 'BOARD_POWER_LIMIT_CONTROL'].min()
        max_control = df['BOARD_POWER_LIMIT_CONTROL'].max()
        X = np.linspace(min_control, args.max_power, 100) / args.max_power
        for profile_name, profile_data in output['profiles'].items():
            is_piecewise = profile_data.get('model_type') == 'piecewise-linear'
            if args.per_host:
                for host_name, host_data in profile_data['hosts'].items():
                    if is_piecewise:
                        pw_pairs = sorted((float(k), float(v)) for k, v in host_data['model'].items())
                        pw_powers = np.array([p for p, _ in pw_pairs])
                        pw_foms = np.array([f for _, f in pw_pairs])
                        fom_max = pw_foms.max()
                        y_pw = fom_max / pw_foms - 1
                        line, = ax.plot(pw_powers / args.max_power, y_pw, marker='.', label=f'{profile_name}@{host_name}')
                    else:
                        y = slowdown_at_power(X, host_data['model']['x0'], host_data['model']['A'], host_data['model']['B'], host_data['model']['C'])
                        line, = ax.plot(X, y, label=f'{profile_name}@{host_name}')
                    if args.show_min_max_range:
                        slowdown_range = df.loc[
                            (df['profile'] == profile_name) & (df['host'] == host_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)
                        ].groupby('BOARD_POWER_LIMIT_CONTROL')['slowdown'].quantile([0, 0.25, 0.75, 1]).unstack()
                        ax.fill_between(slowdown_range.index/args.max_power, slowdown_range.loc[:, 0.25], slowdown_range.loc[:, 0.75], alpha=0.4, color=line.get_color(), linewidth=0)
                        ax.fill_between(slowdown_range.index/args.max_power, slowdown_range.loc[:, 0], slowdown_range.loc[:, 1], alpha=0.1, color=line.get_color(), linewidth=0)
                    if args.show_samples:
                        plot_df = df.loc[(df['profile'] == profile_name) & (df['host'] == host_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)]
                        ax.scatter(plot_df['BOARD_POWER_LIMIT_CONTROL']/args.max_power, plot_df['slowdown'])

            else:
                if is_piecewise:
                    pw_pairs = sorted((float(k), float(v)) for k, v in profile_data['model'].items())
                    pw_powers = np.array([p for p, _ in pw_pairs])
                    pw_foms = np.array([f for _, f in pw_pairs])
                    fom_max = pw_foms.max()
                    y_pw = fom_max / pw_foms - 1
                    line, = ax.plot(pw_powers / args.max_power, y_pw, marker='.', label=profile_name)
                else:
                    y = slowdown_at_power(X, profile_data['model']['x0'], profile_data['model']['A'], profile_data['model']['B'], profile_data['model']['C'])
                    line, = ax.plot(X, y, label=profile_name)
                if args.show_min_max_range:
                    slowdown_range = df.loc[(df['profile'] == profile_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)].groupby(
                            'BOARD_POWER_LIMIT_CONTROL')['slowdown'].quantile([0, 0.25, 0.75, 1]).unstack()
                    ax.fill_between(slowdown_range.index/args.max_power, slowdown_range.loc[:, 0.25], slowdown_range.loc[:, 0.75], alpha=0.4, color=line.get_color(), linewidth=0)
                    ax.fill_between(slowdown_range.index/args.max_power, slowdown_range.loc[:, 0], slowdown_range.loc[:, 1], alpha=0.1, color=line.get_color(), linewidth=0)
                if args.show_samples:
                    plot_df = df.loc[(df['profile'] == profile_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)]
                    ax.scatter(plot_df['BOARD_POWER_LIMIT_CONTROL']/args.max_power, plot_df['slowdown'])
        ax.set_xlabel('Power Cap (w.r.t. max allowed)')
        ax.set_ylabel('Slowdown (w.r.t. 100% power)')
        ax.yaxis.set_major_formatter(mtick.PercentFormatter(1))
        ax.xaxis.set_major_formatter(mtick.PercentFormatter(1))
        if args.show_legend:
            ax.legend()
        fig.savefig(args.plot_path, bbox_inches='tight')


json.dump(output, sys.stdout, indent=2)
print()
