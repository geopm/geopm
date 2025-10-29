#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Creates a 3D plot of the requested metric over the range of gpu frequencies.
'''

import sys
import os
import argparse
import pandas
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm, colors
from matplotlib.ticker import MaxNLocator
from matplotlib.ticker import LogFormatter

import geopmpy.io

from integration.experiment import common_args


def plot_heatmap(data, cmap, norm, zbar_range, zbar_label, z_thresh,
                 x_range, x_label, y_range, y_label, title, outdir, filename):

    # need to transpose the data for imshow
    data = np.array(data)
    data = data.T

    f, ax = plt.subplots()
    im = ax.imshow(data, interpolation='none', cmap=cmap, norm=norm)
    cbar = ax.figure.colorbar(im, ax=ax)
    cbar.set_label(zbar_label, rotation=-90, va='bottom')

    ax.set_xlabel(x_label)
    ax.set_xticklabels(x_range)
    ax.set_xticks(np.arange(len(x_range)))

    ax.set_ylabel(y_label)
    ax.set_yticklabels(y_range)
    ax.set_yticks(np.arange(len(y_range)))

    plt.xticks(rotation=90)  # Rotate x-axis labels by 45 degrees
    plt.yticks(rotation=0)  # Rotate y-axis labels by 45 degrees

    f.set_size_inches(5, 8.0)
    plt.title(title)
    plt.tight_layout()
    filename = '{}.png'.format(filename)
    if not os.path.exists(os.path.join(outdir, 'figures')):
        os.mkdir(os.path.join(outdir, 'figures'))

    plt.gca().invert_yaxis()

    plt.savefig(os.path.join(outdir, 'figures', filename))
    plt.close()


def setup_3d_data(df, metric, show_details, scalar=1e9):
    # rename some columns
    df['runtime'] = df['runtime (s)']
    df['energy'] = df['gpu-energy (J)']
    df['power'] = df['gpu-power (W)']

    df['GPU_CORE_FREQUENCY_MIN_CONTROL'] /= scalar
    xs = sorted(df['BOARD_POWER_LIMIT_CONTROL'].unique())
    ys = sorted(df['GPU_CORE_FREQUENCY_MIN_CONTROL'].unique())

    zs = []
    min_z = df[metric].min()
    max_z = df[metric].max()
    for xx in xs:
        temp = []
        for yy in ys:
            value = df.loc[df['GPU_CORE_FREQUENCY_MIN_CONTROL'] == yy].loc[df['BOARD_POWER_LIMIT_CONTROL'] == xx]
            value = value[metric].mean()
            temp.append(value)
        zs.append(temp)

    if show_details:
        summary = pandas.DataFrame(zs, columns=ys, index=xs)
        summary.index.name = 'gpu-frequency'
        sys.stdout.write('{}\n{}\n'.format(metric, summary))
    return xs, ys, zs, min_z, max_z


def plot_uncore_sweep_heatmap(df, metric, label, region, output_dir, show_details):
    if metric not in ['power', 'energy', 'runtime']:
        raise RuntimeError('Unknown z-axis metric: {}'.format(metric))

    xs, ys, zs, min_z, max_z = setup_3d_data(df, metric, show_details)

    if (metric != 'runtime'):
       min_z = round(min_z-10, -1)
       max_z = round(max_z+10, +1)

    value_range = np.linspace(min_z, max_z, 10)
    z_thresh = 0.0
    cmap = cm.get_cmap('rainbow')
    norm = colors.Normalize(value_range.min(), value_range.max())

    metric_label = { 'runtime': 'Application Runtime (sec)',
                     'energy': 'GPU Energy Consumption (J)',
                     'power': 'GPU Power Consumption (W)'}

    plot_heatmap(data=zs, cmap=cmap, norm=norm, zbar_range=value_range,
                 zbar_label=metric_label[metric], z_thresh=z_thresh,
                 x_range=xs, x_label='Requested Platform Power Cap (W)',
                 y_range=ys, y_label='Requested GPU Frequency (GHz)',
                 title='{} Frequency Sweep under Platform Power Caps\nregion = {}, z-axis = {}'.format(label, region, metric),
                 outdir=output_dir,
                 filename='{}_platform_power_frequency_sweep_{}_{}'.format(label.lower(), metric, region.lower()))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    parser.add_argument('--metric', dest='metric',
                        action='store', type=str, default='power',
                        help='metric to use for z-axis, one of power, energy, or runtime (default: power)')
    parser.add_argument('--hostname', required=True, dest='hostname',
                        action='store', default=None,
                        help='Hostname of the node to which the report belongs')

    args = parser.parse_args()
    output_dir = args.output_dir
    regex_report_format = f"*_trial_1_*{args.hostname}*report"
    print(f"Filename = {regex_report_format}")
    try:
        output = geopmpy.io.RawReportCollection(regex_report_format, dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    label = "GPU"
    data = output.get_app_df()
    region = "Totals"

    plot_uncore_sweep_heatmap(df=data, metric=args.metric,
                                  label=label, region=region, output_dir=args.output_dir,
                                  show_details=args.show_details)
