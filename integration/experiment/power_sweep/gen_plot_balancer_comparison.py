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
Example power sweep experiment using geopmbench.
'''

import sys
import os
import pandas
import argparse
import matplotlib.pyplot as plt
import numpy

import geopmpy.io

from experiment import common_args


def prep_plot_data(report_data, metric, normalize, speedup):
    edf = report_data

    # rename some columns
    edf['power_limit'] = edf['POWER_PACKAGE_LIMIT_TOTAL']
    edf['runtime'] = edf['runtime (sec)']
    edf['network_time'] = edf['network-time (sec)']
    edf['energy_pkg'] = edf['package-energy (joules)']
    edf['frequency'] = edf['frequency (Hz)']

    edf = edf.set_index(['Agent', 'power_limit', 'Profile', 'host'])

    df = pandas.DataFrame()
    reference = 'power_governor'
    target = 'power_balancer'

    ref_epoch_data = edf.xs([reference])
    if metric == 'power':
        rge = ref_epoch_data['energy_pkg']
        rgr = ref_epoch_data['runtime']
        reference_g = (rge / rgr).groupby(level='power_limit')
    else:
        reference_g = ref_epoch_data[metric].groupby(level='power_limit')

    df['reference_mean'] = reference_g.mean()
    df['reference_max'] = reference_g.max()
    df['reference_min'] = reference_g.min()

    tar_epoch_data = edf.xs([target])
    if metric == 'power':
        tge = tar_epoch_data['energy_pkg']
        tgr = tar_epoch_data['runtime']
        target_g = (tge / tgr).groupby(level='power_limit')
    else:
        target_g = tar_epoch_data[metric].groupby(level='power_limit')

    df['target_mean'] = target_g.mean()
    df['target_max'] = target_g.max()
    df['target_min'] = target_g.min()

    if normalize and not speedup:  # Normalize the data against the rightmost reference bar
        df /= df['reference_mean'].iloc[-1]

    if speedup:  # Plot the inverse of the target data to show speedup as a positive change
        df = df.div(df['reference_mean'], axis='rows')
        df['target_mean'] = 1 / df['target_mean']
        df['target_max'] = 1 / df['target_max']
        df['target_min'] = 1 / df['target_min']

    # Convert the maxes and mins to be deltas from the mean; required for the errorbar API
    df['reference_max_delta'] = df['reference_max'] - df['reference_mean']
    df['reference_min_delta'] = df['reference_mean'] - df['reference_min']
    df['target_max_delta'] = df['target_max'] - df['target_mean']
    df['target_min_delta'] = df['target_mean'] - df['target_min']

    return df


def plot_balancer_comparison(df, label, metric, output_dir='.',
                             speedup=False, normalize=False, detailed=False):

    units = {
        'energy': 'J',
        'energy_pkg': 'J',
        'runtime': 's',
        'frequency': '% of sticker',
        'power': 'W',
    }

    # Analysis
    df = prep_plot_data(df, metric=metric, normalize=normalize, speedup=speedup)
    if detailed:
        sys.stdout.write('{}\n'.format(df))

    # Plotting
    output_types = ['png']

    target_agent = 'power_balancer'
    reference_agent = 'power_governor'

    # Begin plot setup
    f, ax = plt.subplots()
    bar_width = 0.35
    index = numpy.arange(min(len(df['target_mean']), len(df['reference_mean'])))

    plt.bar(index - bar_width / 2,
            df['reference_mean'],
            width=bar_width,
            color='blue',
            align='center',
            label=reference_agent.replace('_', ' ').title(),
            zorder=3)

    ax.errorbar(index - bar_width / 2,
                df['reference_mean'],
                xerr=None,
                yerr=(df['reference_min_delta'], df['reference_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    plt.bar(index + bar_width / 2,
            df['target_mean'],
            width=bar_width,
            color='cyan',
            align='center',
            label=target_agent.replace('_', ' ').title(),
            zorder=3)  # Forces grid lines to be drawn behind the bar

    ax.errorbar(index + bar_width / 2,
                df['target_mean'],
                xerr=None,
                yerr=(df['target_min_delta'], df['target_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    ax.set_xticks(index)
    xlabels = [int(xx) for xx in df.index]
    ax.set_xticklabels(xlabels)
    ax.set_xlabel('Average Node Power Limit (W)')

    if metric == 'energy_pkg':
        title_datatype = 'Energy'
    else:
        title_datatype = metric.title()

    ylabel = title_datatype
    if normalize and not speedup:
        ylabel = 'Normalized {}'.format(ylabel)
    elif not normalize and not speedup:
        units_label = units.get(metric)
        ylabel = '{}{}'.format(ylabel, ' ({})'.format(units_label) if units_label else '')
    else:  # if speedup:
        ylabel = 'Normalized Speed-up'
    ax.set_ylabel(ylabel)

    ax.grid(axis='y', linestyle='--', color='black')

    plt.title('{}: {} Decreases from Power Balancing'.format(label, title_datatype), y=1.02)

    plt.margins(0.02, 0.01)
    plt.axis('tight')
    legend_fontsize = 14
    plt.legend(shadow=True, fancybox=True, fontsize=legend_fontsize, loc='best').set_zorder(11)
    plt.tight_layout()

    if speedup:
        yspan = 0.35
        # Check yspan before setting to ensure span is > speedup
        abs_max_val = max(abs(df['target_mean'].max()), abs(df['target_mean'].min()))
        abs_max_val = abs(abs_max_val - 1)
        if abs_max_val > yspan:
            yspan = abs_max_val * 1.1
        ax.set_ylim(1 - yspan, 1 + yspan)
    else:
        ymax = ax.get_ylim()[1]
        ymax *= 1.1
        ax.set_ylim(0, ymax)

    # Write data/plot files
    file_name = '{}_{}_comparison'.format(label.lower().replace(' ', '_'), metric)
    if speedup:
        # speedup alone and normalized speedup are the same
        file_name += '_speedup'
    elif normalize:
        file_name += '_normalized'
    if detailed:
        sys.stdout.write('Writing:\n')
    for ext in output_types:
        if not os.path.exists(os.path.join(output_dir, 'figures')):
            os.mkdir(os.path.join(output_dir, 'figures'))
        full_path = os.path.join(output_dir, 'figures', '{}.{}'.format(file_name, ext))
        plt.savefig(full_path)
        if detailed:
            sys.stdout.write('    {}\n'.format(full_path))
    sys.stdout.flush()
    plt.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_label(parser)
    parser.add_argument('--normalize', action='store_true', default=False,
                        help='use normalized values')
    parser.add_argument('--speedup', action='store_true', default=False,
                        help='show results as a speedup percentage')
    parser.add_argument('--metric', action='store', default='runtime',
                        help='metric to use for comparison.  One of: runtime, energy_pkg, frequency, power')

    args = parser.parse_args()
    output_dir = args.output_dir

    # TODO: make into utility function
    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a power sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    plot_balancer_comparison(output.get_epoch_df(),
                             label=args.label,
                             metric=args.metric,
                             output_dir=output_dir,
                             normalize=args.normalize,
                             speedup=args.speedup,
                             detailed=args.show_details)
