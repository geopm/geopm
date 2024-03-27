#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import seaborn as sns
import pandas as pd
import matplotlib.ticker as mtick
import os

parser = argparse.ArgumentParser()
parser.add_argument('csvs', nargs='+',
                    help='One or more CSV files containing DURATION of each batch operation.')
parser.add_argument('--plot-path', default='./geopm_batch_read_latency.png',
                    help='Where to save the generated plot.')
parser.add_argument('--min-y', type=float, default=0.0001,
                    help='Minimum value to show on the y axis, in seconds.')

args = parser.parse_args()

sns.set()

configs = [os.path.splitext(os.path.basename(csv_path))[0] for csv_path in args.csvs]
df = pd.concat([pd.read_csv(csv_path) for csv_path in args.csvs], keys=configs, names=['Config', 'sample'])
df[['Backend', 'Privilege', 'Trial']] = (df.index.get_level_values('Config')
                                        .to_series(index=df.index).str.rsplit(pat='-', n=2, expand=True))
df['Which Iteration'] = 'First'
df.loc[(slice(None), slice(1, None)), 'Which Iteration'] = 'Rest'
df['Latency (ms)'] = df['DURATION'] * 1000
g = sns.catplot(
    data=df.reset_index(),
    kind='bar',
    col='Backend',
    col_order=df.loc[df['Which Iteration'] == 'Rest'].groupby('Backend')['DURATION'].mean().sort_values().index,
    x='Privilege',
    aspect=0.5,
    hue='Which Iteration',
    ci='sd',
    y='DURATION',
)
g.set(ylim=(args.min_y, None), xlabel=None)
g.set_axis_labels(y_var='Latency')
for ax in g.axes.flat:
    ax.set_yscale('log')
    ax.grid(True, which="both", axis='y')
    locmin = mtick.LogLocator(base=10, numticks=10)
    ax.xaxis.set_minor_locator(locmin)
    ax.yaxis.set_major_formatter(mtick.EngFormatter('s'))
g.tight_layout()
g.savefig(args.plot_path, bbox_inches='tight')
