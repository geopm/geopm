#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import argparse
import pandas as pd
import numpy as np
import sklearn.metrics
from scipy import optimize
import json
import sys
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
parser.add_argument('--reports', nargs='+')
parser.add_argument('-v', '--verbose', action='store_true',
                    help='Print additional information about coefficient selection.')
parser.add_argument('--plot-path',
                    help='Path to save a plot of the power-performance data. Default: no plots are generated')

group = parser.add_mutually_exclusive_group()
group.add_argument('--use-region', help='Use time spent in the given region.')
group.add_argument('--use-fom', action='store_true', help='Use the Figure of Merit')

args = parser.parse_args()


def get_coefficients(df):
    """Given a DataFrame containing power and performance data, return a tuple
    of model coefficients that fit to the data.

    Returns: x0, A, B, C
    """
    columns = ['agent', 'profile', 'host', 'slowdown', 'BOARD_POWER_LIMIT_CONTROL', 'runtime (s)', 'BOARD_ENERGY']
    if not args.use_fom:
        df = df[columns][df['agent'] == 'node_power_governor']
    else:
        df = df[columns + ['FOM']][df['agent'] == 'node_power_governor']

    X = np.reshape(df['BOARD_POWER_LIMIT_CONTROL'].values, (-1, 1)).astype('float64') / args.max_power
    y = df['slowdown'].values

    res = optimize.minimize(
        loss,
        # Initial guess: x0=1 (max power), everything else is zero (i.e., a flat line)
        [1, 0, 0, 0],
        args=(y, X),
        # y = A * (x0 - x)**2 + B * (x0 - x) + C
        # dy/dx: -A*2*x0 + A*2 - B
        constraints=(
            # Constraint: Slowdown decreases as power increases (2*x0*A - 2*A*x + B >= 0)
            dict(type='ineq', fun=lambda x: 2 * x[0] * x[1] - 2 * x[1] * X.flatten() + x[2]),
            dict(type='ineq', fun=lambda x: x[0]),  # X0 >= 0
            dict(type='ineq', fun=lambda x: x[1]),  # A >= 0
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


data_list = list()
for report_path in args.reports:
    with open(report_path) as f:
        report = load(f, Loader=SafeLoader)
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
    if args.per_host:
        output['profiles'][profile] = dict(hosts=dict())
        for host, df_host in df_profile.groupby('host'):
            output['profiles'][profile]['hosts'][host] = dict(
                model=dict(zip(('x0', 'A', 'B', 'C'), get_coefficients(df_host))))
    else:
        output['profiles'][profile] = dict(
            model=dict(zip(('x0', 'A', 'B', 'C'), get_coefficients(df_profile))))

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
        X = np.linspace(min_control, max(args.max_power, max_control), 100) / args.max_power
        for profile_name, profile_data in output['profiles'].items():
            if args.per_host:
                for host_name, host_data in profile_data['hosts'].items():
                    y = slowdown_at_power(X, host_data['model']['x0'], host_data['model']['A'], host_data['model']['B'], host_data['model']['C'])
                    ax.plot(X, y, label=f'{profile_name}@{host_name}')
                    if args.show_min_max_range:
                        slowdown_range = df.loc[
                            (df['profile'] == profile_name) & (df['host'] == host_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)
                        ].groupby('BOARD_POWER_LIMIT_CONTROL')['slowdown'].agg(['min', 'max'])
                        ax.fill_between(slowdown_range.index/args.max_power, slowdown_range['min'], slowdown_range['max'], alpha=0.5)
                    if args.show_samples:
                        plot_df = df.loc[(df['profile'] == profile_name) & (df['host'] == host_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)]
                        ax.scatter(plot_df['BOARD_POWER_LIMIT_CONTROL']/args.max_power, plot_df['slowdown'])

            else:
                y = slowdown_at_power(X, profile_data['model']['x0'], profile_data['model']['A'], profile_data['model']['B'], profile_data['model']['C'])
                ax.plot(X, y, label=profile_name)
                if args.show_min_max_range:
                    slowdown_range = df.loc[(df['profile'] == profile_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)].groupby(
                            'BOARD_POWER_LIMIT_CONTROL')['slowdown'].agg(['min', 'max'])
                    ax.fill_between(slowdown_range.index/args.max_power, slowdown_range['min'], slowdown_range['max'], alpha=0.5)
                if args.show_samples:
                    plot_df = df.loc[(df['profile'] == profile_name) & (df['BOARD_POWER_LIMIT_CONTROL'] != 0)]
                    ax.scatter(plot_df['BOARD_POWER_LIMIT_CONTROL']/args.max_power, plot_df['slowdown'])
        ax.set_xlabel('Power Cap (w.r.t. max allowed)')
        ax.set_ylabel('Slowdown (w.r.t. 100% power)')
        ax.yaxis.set_major_formatter(mtick.PercentFormatter(1))
        ax.xaxis.set_major_formatter(mtick.PercentFormatter(1))
        ax.legend()
        fig.savefig(args.plot_path, bbox_inches='tight')


json.dump(output, sys.stdout, indent=2)
print()
