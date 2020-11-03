#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

'''
Creates a 3D plot of the requested metric over the range of core and
uncore frequencies.
'''

import sys
import os
import argparse
import pandas
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm, colors

import geopmpy.io

from experiment import common_args


def plot_3d(df):
    pass


def plot_heatmap(data, cmap, norm, zbar_range, zbar_label, z_thresh,
                 x_range, x_label, y_range, y_label, title, outdir):

    # need to transpose the data for imshow
    data = np.array(data)
    data = data.T

    f, ax = plt.subplots()
    im = ax.imshow(data, interpolation='none', cmap=cmap, norm=norm)
    cbar = ax.figure.colorbar(im, ax=ax, ticks=zbar_range)
    cbar.ax.set_ylabel(zbar_label, rotation=-90, va='bottom')

    ax.set_xlabel(x_label)
    ax.set_xticklabels(x_range, rotation=90)
    ax.set_xticks(np.arange(len(x_range)))

    ax.set_ylabel(y_label)
    ax.set_yticklabels(y_range)
    ax.set_yticks(np.arange(len(y_range)))

    # display text value over color
    for x in range(len(x_range)):
        for y in range(len(y_range)):
            # note reversal of x and y for data access
            color = 'black'
            if data[y][x] < z_thresh:
                color = 'white'
            ax.text(x, y, '{:.0f}'.format(data[y][x]), ha='center', va='center',
                    color=color, size=8)

    ######################
    # fix for top and bottom being cut off
    b, t = ax.get_ylim()
    b += 0.5
    t -= 0.5
    ax.set_ylim(b, t)
    ################

    plt.title(title)
    f.tight_layout()
    filename = '{}.png'.format(title.replace(' ', '_').replace('\n', '_').replace(':', ''))
    if not os.path.exists(os.path.join(outdir, 'figures')):
        os.mkdir(os.path.join(outdir, 'figures'))
    plt.savefig(os.path.join(outdir, 'figures', filename))
    plt.close()


def setup_3d_data(df, metric, show_details):
    # rename some columns
    df['runtime'] = df['runtime (sec)']
    df['energy'] = df['package-energy (joules)']
    df['power'] = df['power (watts)']

    xs = sorted(df['FREQ_DEFAULT'].unique())
    ys = sorted(df['FREQ_UNCORE'].unique())
    zs = []
    min_z = df[metric].min()
    max_z = df[metric].max()
    for xx in xs:
        temp = []
        for yy in ys:
            value = df.loc[df['FREQ_DEFAULT'] == xx].loc[df['FREQ_UNCORE'] == yy]
            value = value[metric].mean()
            temp.append(value)
        zs.append(temp)
    if show_details:
        summary = pandas.DataFrame(zs, columns=ys, index=xs)
        summary.index.name = 'core_frequency'
        sys.stdout.write('{}\n{}\n'.format(metric, summary))
    return xs, ys, zs, min_z, max_z


def plot_uncore_sweep_heatmap(df, metric, label, output_dir, show_details):
    if metric not in ['power', 'energy', 'runtime']:
        raise RuntimeError('Unknown z-axis metric: {}'.format(metric))

    xs, ys, zs, min_z, max_z = setup_3d_data(df, metric, show_details)
    value_range = np.linspace(min_z, max_z, 20)
    z_thresh = 0.0
    cmap = cm.get_cmap('autumn')
    norm = colors.Normalize(value_range.min(), value_range.max())

    plot_heatmap(data=zs, cmap=cmap, norm=norm, zbar_range=value_range,
                 zbar_label=metric, z_thresh=z_thresh,
                 x_range=xs, x_label='Core Frequency (Hz)',
                 y_range=ys, y_label='Uncore Frequency (Hz)',
                 title='{} Uncore Frequency Sweep: {}'.format(label, metric),
                 outdir=output_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_label(parser)
    parser.add_argument('--target-region', dest='target_region',
                        action='store', type=str, default=None,
                        help='name of the region to use to select data (default=application totals)')
    parser.add_argument('--metric', dest='metric',
                        action='store', type=str, default='power',
                        help='metric to use for z-axis, one of power, energy, or runtime (default: power)')
    parser.add_argument('--plot-3d', dest='plot_3d',
                        action='store_true', default=False,
                        help='plot as a 3D surface instead of a heatmap')

    args = parser.parse_args()
    output_dir = args.output_dir

    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    label = args.label
    target_region = args.target_region
    if target_region is not None:
        df = output.get_df()
        data = df.loc[df['region'] == target_region]
        label += ', ' + target_region
    else:
        data = output.get_app_df()

    if args.plot_3d:
        plot_3d(data)
    else:
        plot_uncore_sweep_heatmap(df=data, metric=args.metric,
                                  label=args.label, output_dir=args.output_dir,
                                  show_details=args.show_details)
