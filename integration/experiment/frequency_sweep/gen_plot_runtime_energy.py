#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Creates a plot of the runtime and energy of a region of interest
versus the fixed frequency from a frequency sweep.
'''

import sys
import os
import argparse
import matplotlib.pyplot as plt

import geopmpy.io

from integration.experiment import common_args
from integration.experiment import report
from integration.experiment import plotting


def generate_runtime_energy_plot(df, perf_metric_label, energy_metric_label, title, output_dir='.'):
    """
    Creates a plot comparing the runtime and energy of a region on two axes.
    """
    errorbar_format = {'fmt': ' ',
                       'label': '',
                       'elinewidth': 1,
                       'zorder': 10}

    f, ax = plt.subplots()
    points = df.index

    ax.plot(points, df['energy_perf'], color='purple', label=energy_metric_label, marker='o', linestyle='-')
    ax.set_ylabel(energy_metric_label)
    yerr = (df['min_delta_energy'], df['max_delta_energy'])
    ax.errorbar(points, df['energy_perf'], xerr=None,
                yerr=yerr, color='purple', **errorbar_format)

    ax2 = ax.twinx()
    ax2.plot(points, df['performance'], color='orange', label=perf_metric_label, marker='s', linestyle='-')
    ax2.set_ylabel(perf_metric_label)
    yerr = (df['min_delta_perf'], df['max_delta_perf'])
    ax2.errorbar(points, df['performance'], xerr=None,
                 yerr=yerr, color='orange', **errorbar_format)

    ax.set_xlabel('Frequency (GHz)')
    lines, labels = ax.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()

    plt.legend(lines + lines2, labels + labels2, shadow=True, fancybox=True, loc='best')
    plt.title('{}'.format(title))
    f.tight_layout()
    if not os.path.exists(os.path.join(output_dir, 'figures')):
        os.mkdir(os.path.join(output_dir, 'figures'))
    figname = plotting.title_to_filename('{}_freq_energy'.format(title))
    plt.savefig(os.path.join(output_dir, 'figures', figname + '.png'))
    plt.close()


def find_longest_region(report_df):
    max_freq = report_df['freq_hz'].max()
    max_freq_data = report_df.loc[report_df['freq_hz'] == max_freq]
    longest_runtime = max_freq_data['runtime'].max()
    longest_data = max_freq_data.loc[max_freq_data['runtime'] == longest_runtime]
    regions = longest_data['region'].unique()
    longest = regions[0]
    if len(regions) > 1:
        sys.stderr.write('<geopm> Warning: multiple regions with same longest runtime.')
    return longest


# TODO: some repeated code in profile comparison (without normalization)
def plot_runtime_energy(report_df, perf_metric, use_stdev, label, output_dir, show_details=False):

    report.prepare_columns(report_df)
    report.prepare_metrics(report_df, perf_metric)
    # TODO: might want loop_key to be passed in as a function instead
    freqs = sorted(report_df['FREQ_CPU_DEFAULT'].unique())
    result = report.energy_perf_summary(df=report_df,
                                        loop_key='FREQ_CPU_DEFAULT',
                                        loop_vals=freqs,
                                        baseline=None,
                                        perf_metric=perf_metric,
                                        use_stdev=use_stdev)
    perf_metric_label, energy_metric_label = report.perf_metric_label(perf_metric)

    if show_details:
        sys.stdout.write('Data for {}:\n{}\n'.format(label, result))

    title = label
    if use_stdev:
        title += ' (stdev)'
    else:
        title += ' (min-max)'
    generate_runtime_energy_plot(result, perf_metric_label, energy_metric_label, title, output_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_label(parser)
    common_args.add_use_stdev(parser)
    common_args.add_performance_metric(parser)
    parser.add_argument('--target-region', dest='target_region',
                        action='store', type=str, default=None,
                        help='name of the region to use to select data (default=application totals)')

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
        # Test if Epochs were used
        if output.get_epoch_df()[:1]['count'].item() > 0.0:
            data = output.get_epoch_df()
        else:
            data = output.get_app_df()

    label += ' ' + args.performance_metric

    plot_runtime_energy(report_df=data,
                        use_stdev=args.use_stdev,
                        label=label,
                        output_dir=output_dir,
                        show_details=args.show_details,
                        perf_metric=args.performance_metric)
