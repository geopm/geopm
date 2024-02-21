#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import pandas as pd
import argparse
import seaborn as sns
import os
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import math
from scipy import optimize
import sklearn.metrics
import numpy as np
import sys

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


def get_coefficients(df):
    """Given a DataFrame containing power and performance data, return a tuple
    of model coefficients that fit to the data.

    Returns: x0, A, B, C
    """
    X = np.reshape(df['GPU Power Cap'].values, (-1, 1)).astype('float64') / df['GPU Power Cap'].max()
    y = df['Slowdown'].values

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

    y_train_pred = slowdown_at_power(X, *params)
    r2score = sklearn.metrics.r2_score(y, y_train_pred)
    print(f'Training R2 Score: {r2score}', file=sys.stderr)

    return params

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('csv_paths', nargs='+')
    parser.add_argument('--plot-path',
                        help='Directory to which all plots will be saved.')
    parser.add_argument('--power-at-slowdown',
                        type=float,
                        help='Print the power cap that would achieve the given slowdown. '
                             '0 indicates max speed. 1.0 indicates 100% slowdown (2x time)')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Print extra information to stdout')
    parser.add_argument('--extension',
                        default='png',
                        help='File extension for images')
    args = parser.parse_args()
    
    samples = list()
    for csv_path in args.csv_paths:
        if args.verbose:
            print(csv_path)
        df = pd.read_csv(csv_path)
        df.columns = [c.split('-', 1)[0] if '-' in c else c for c in df.columns]
        power_cap = df['GPU_POWER_LIMIT_CONTROL'].mean()
        if args.verbose:
            print(power_cap)
        samples.append({
            'GPU Power Cap': power_cap,
            'GEOPM Time': df['TIME'].iloc[-1] - df['TIME'].iloc[0],
            'GPU Energy': df['GPU_ENERGY'].iloc[-1] - df['GPU_ENERGY'].iloc[0],
            'CPU Energy': df['CPU_ENERGY'].iloc[-1] - df['CPU_ENERGY'].iloc[0],
            'DRAM Energy': df['DRAM_ENERGY'].iloc[-1] - df['DRAM_ENERGY'].iloc[0],
            'GPU Frequency': df['GPU_CORE_FREQUENCY_STATUS'].mean(),
            'CPU Frequency': df['CPU_FREQUENCY_STATUS'].mean(),
        })
    
    report_df = pd.DataFrame(samples)
    report_df['GPU Power'] = report_df['GPU Energy'] / report_df['GEOPM Time']
    report_df['CPU Power'] = report_df['CPU Energy'] / report_df['GEOPM Time']
    report_df['DRAM Power'] = report_df['DRAM Energy'] / report_df['GEOPM Time']
    report_df['Energy'] = report_df['GPU Energy'] + report_df['CPU Energy'] + report_df['DRAM Energy']
    report_df['Slowdown'] = report_df['GEOPM Time'] / report_df['GEOPM Time'].min() - 1
    
    model_coefficients = get_coefficients(report_df)
    print('Coefficients', model_coefficients)
    
    if args.power_at_slowdown is not None:
        print(report_df['GPU Power Cap'].max() * power_at_slowdown(args.power_at_slowdown, *model_coefficients), 'W')
    
    if args.plot_path is not None:
        sns.set(context='talk',
                style='ticks',
                )
        os.makedirs(args.plot_path, exist_ok=True)
        fig, ax = plt.subplots(figsize=(7, 5))
        X = np.linspace(report_df['GPU Power Cap'].min(), report_df['GPU Power Cap'].max(), 100)
        y = slowdown_at_power(X / report_df['GPU Power Cap'].max(), *model_coefficients)
    
        relenergy = report_df['Energy'] / report_df['Energy'].max()
        relenergysaved = 1 - relenergy
        ax.plot(report_df['GPU Power Cap'], report_df['Slowdown'], '.', label='Performance Degradation')
        ax.plot(report_df['GPU Power Cap'], relenergysaved, 'x', label='CPU+GPU+DRAM Energy Saved')
        ax.plot(X, y, 'b--', label='Modeled Slowdown')
        ax.set_xlabel('GPU Power Cap (W)')
        ax.set_ylabel('Relative Measurement')
        ax.legend(loc='upper right')
        ax.yaxis.set_major_formatter(mtick.PercentFormatter(1))
        fig.savefig(os.path.join(args.plot_path, f'gpu_power_cap_vs_reltime.{args.extension}'),
                    bbox_inches="tight")
        plt.close(fig)
