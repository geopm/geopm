#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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

from integration.experiment import common_args
from integration.experiment import report
from integration.experiment import plotting


def prep_plot_data(report_data, metric, normalize, speedup, use_stdev):
    idf = report_data
    report.prepare_columns(idf)

    # rename some columns
    idf['power_limit'] = idf['CPU_POWER_LIMIT']

    # mean across nodes within trials
    idf = idf.set_index(['Agent', 'power_limit', 'host', 'trial'])
    idf = idf.groupby(['Agent', 'power_limit', 'trial']).mean()

    df = pandas.DataFrame()
    reference = 'power_governor'
    target = 'power_balancer'

    ref_epoch_data = idf.xs([reference])

    if metric == 'power':
        rge = ref_epoch_data['energy'] # Added by report.prepare_columns
        rgr = ref_epoch_data['runtime']
        reference_g = (rge / rgr).groupby(level='power_limit')
    else:
        reference_g = ref_epoch_data[metric].groupby(level='power_limit')

    df['reference_mean'] = reference_g.mean()
    df['reference_max'] = reference_g.max()
    df['reference_min'] = reference_g.min()
    df['reference_std'] = reference_g.std()

    tar_epoch_data = idf.xs([target])
    if metric == 'power':
        tge = tar_epoch_data['energy']
        tgr = tar_epoch_data['runtime']
        target_g = (tge / tgr).groupby(level='power_limit')
    else:
        target_g = tar_epoch_data[metric].groupby(level='power_limit')

    df['target_mean'] = target_g.mean()
    df['target_max'] = target_g.max()
    df['target_min'] = target_g.min()
    df['target_std'] = target_g.std()

    if normalize and not speedup:  # Normalize the data against the rightmost reference bar
        df /= df['reference_mean'].iloc[-1]

    if speedup:  # Plot the inverse of the target data to show speedup as a positive change
        df = df.div(df['reference_mean'], axis='rows')
        df['target_mean'] = 1 / df['target_mean']
        df['target_max'] = 1 / df['target_max']
        df['target_min'] = 1 / df['target_min']
        # do not invert stdev

    # Convert the maxes and mins to be deltas from the mean; required for the errorbar API
    if use_stdev:
        df['reference_min_delta'] = df['reference_std']
        df['reference_max_delta'] = df['reference_std']
        df['target_min_delta'] = df['target_std']
        df['target_max_delta'] = df['target_std']
    else:
        df['reference_max_delta'] = df['reference_max'] - df['reference_mean']
        df['reference_min_delta'] = df['reference_mean'] - df['reference_min']
        df['target_max_delta'] = df['target_max'] - df['target_mean']
        df['target_min_delta'] = df['target_mean'] - df['target_min']

    return df


def plot_balancer_comparison(output, label, metric, output_dir='.',
                             speedup=False, normalize=False, use_stdev=False,
                             detailed=False):

    units = {
        'energy': 'J',
        'energy': 'J',
        'runtime': 's',
        'frequency': '% of sticker',
        'power': 'W',
    }

    # Test if Epochs were used
    if output.get_epoch_df()[:1]['count'].item() > 0.0:
        df = output.get_epoch_df()
        df_type = "Epochs"
    else:
        df = output.get_app_df()
        df_type = "App Totals"
    label += ' ' + df_type

    # Analysis
    df = prep_plot_data(df, metric=metric, normalize=normalize, speedup=speedup,
                        use_stdev=use_stdev)

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

    if metric == 'energy':
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

    plt.title('{} : Governor vs. Balancer {}'.format(label, title_datatype), y=1.02)

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
    file_name = '{}_{}_comparison'.format(label, metric)
    if speedup:
        # speedup alone and normalized speedup are the same
        file_name += '_speedup'
    elif normalize:
        file_name += '_normalized'
    if use_stdev:
        file_name += '_stdev'
    else:
        file_name += '_minmax'
    file_name = plotting.title_to_filename(file_name)
    if detailed:
        sys.stdout.write('{}\n'.format(df))
        sys.stdout.write('Writing:\n')
    if not os.path.exists(os.path.join(output_dir, 'figures')):
        os.mkdir(os.path.join(output_dir, 'figures'))
    full_path = os.path.join(output_dir, 'figures', file_name)
    plt.savefig(full_path + '.png')
    if detailed:
        sys.stdout.write('    {}\n'.format(full_path + '.png'))
        sys.stdout.write('    {}\n'.format(full_path + '.log'))
        with open(full_path + '.log', 'w') as log:
            log.write('{}\n'.format(df))
    sys.stdout.flush()
    plt.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_label(parser)
    common_args.add_use_stdev(parser)
    parser.add_argument('--normalize', action='store_true', default=False,
                        help='use normalized values')
    parser.add_argument('--speedup', action='store_true', default=False,
                        help='show results as a speedup percentage')
    parser.add_argument('--metric', action='store', default='runtime',
                        help='metric to use for comparison.  One of: runtime, energy, frequency, power, FOM') # For these plots, energy means total socket energy

    args = parser.parse_args()
    output_dir = args.output_dir

    # TODO: make into utility function
    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a power sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    plot_balancer_comparison(output,
                             label=args.label,
                             metric=args.metric,
                             output_dir=output_dir,
                             normalize=args.normalize,
                             speedup=args.speedup,
                             use_stdev=args.use_stdev,
                             detailed=args.show_details)
