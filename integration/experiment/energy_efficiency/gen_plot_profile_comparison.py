#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Plot runtime and energy differences vs. a baseline for multiple profile names.
The baseline and target data can be from any agent, policy, or application.
Multiple iterations of the same profile will be combined by removing the
iteration number after the last underscore from the profile.
'''

import argparse
import sys
import os
import numpy
import matplotlib.pyplot as plt

import geopmpy.io
import geopmpy.hash
from experiment import common_args
from experiment import report
from experiment import plotting


def summary(df, perf_metric, use_stdev, baseline, targets, show_details):

    report.prepare_columns(df)
    report.prepare_metrics(df, perf_metric)

    result = report.energy_perf_summary(df=df,
                                        loop_key='Profile',
                                        loop_vals=targets,
                                        baseline=baseline,
                                        perf_metric=perf_metric,
                                        use_stdev=use_stdev)

    output_prefix = os.path.join(output_dir, '{}'.format(common_prefix))
    output_stats_name = '{}_stats.log'.format(output_prefix)

    if show_details:
        sys.stdout.write('{}\n'.format(result))
    with open(output_stats_name, 'w') as outfile:
        outfile.write('{}\n'.format(result))

    return result


def plot_bars(df, baseline_profile, title, perf_label, energy_label, xlabel, output_dir, show_details):

    labels = df.index.format()
    prefix = os.path.commonprefix(labels)
    labels = list(map(lambda x: x[len(prefix):], labels))
    points = numpy.arange(len(df.index))
    bar_width = 0.35

    errorbar_format = {'fmt': ' ',  # no connecting line
                       'label': '',
                       'color': 'r',
                       'elinewidth': 2,
                       'capthick': 2,
                       'zorder': 10}

    f, ax = plt.subplots()
    # plot performance
    perf = df['performance']
    yerr = (df['min_delta_perf'], df['max_delta_perf'])
    ax.bar(points - bar_width / 2, perf, width=bar_width, color='orange',
           label=perf_label)
    ax.errorbar(points - bar_width / 2, perf, xerr=None,
                yerr=yerr, **errorbar_format)

    # plot energy
    ax.bar(points + bar_width / 2, df['energy_perf'], width=bar_width, color='purple',
           label=energy_label)
    yerr = (df['min_delta_energy'], df['max_delta_energy'])
    ax.errorbar(points + bar_width / 2, df['energy_perf'], xerr=None,
                yerr=yerr, **errorbar_format)

    # baseline
    ax.axhline(y=1.0, color='blue', linestyle='dotted', label='baseline')

    ax.set_xticks(points)
    ax.set_xticklabels(labels, rotation='vertical')
    ax.set_xlabel(xlabel)
    f.subplots_adjust(bottom=0.25)  # TODO: use longest profile name
    ax.set_ylabel('Normalized performance or energy')
    plt.title('{}\nBaseline: {}'.format(title, baseline_profile))
    plt.legend()
    fig_dir = os.path.join(output_dir, 'figures')
    if not os.path.exists(fig_dir):
        os.mkdir(fig_dir)
    fig_name = plotting.title_to_filename(title)
    fig_name = os.path.join(fig_dir, fig_name + '.png')
    plt.savefig(fig_name)
    if show_details:
        sys.stdout.write('Wrote {}\n'.format(fig_name))


def get_profile_list(rrc):
    result = rrc.get_app_df()['Profile'].unique()
    profs, _ = zip(*map(report.extract_trial, result))
    result = sorted(list(set(profs)))
    return result


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_use_stdev(parser)
    common_args.add_performance_metric(parser)

    parser.add_argument('--baseline', dest='baseline',
                        action='store', default=None,
                        help='baseline profile name')
    parser.add_argument('--targets', dest='targets',
                        action='store', default=None,
                        help='comma-separated list of profile names to compare')
    parser.add_argument('--list-profiles', dest='list_profiles',
                        action='store_true', default=False,
                        help='list all profiles present in the discovered reports and exit')
    parser.add_argument('--xlabel', dest='xlabel',
                        action='store', default='Profile',
                        help='x-axis label for profiles')

    args = parser.parse_args()

    output_dir = args.output_dir
    try:
        rrc = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except RuntimeError as ex:
        sys.stderr.write('<geopm> Error: No report data found in {}: {}\n'.format(output_dir, ex))
        sys.exit(1)

    profile_list = get_profile_list(rrc)
    if args.list_profiles:
        sys.stdout.write(','.join(profile_list) + '\n')
        sys.exit(0)

    if args.baseline:
        baseline = args.baseline
    else:
        if len(profile_list) < 3:
            raise RuntimeError('Fewer than 3 distinct profiles discovered, --baseline must be provided')
        longest = 0
        for pp in profile_list:
            without = list(profile_list)
            without.remove(pp)
            rank = len(os.path.commonprefix(without))
            if longest < rank:
                longest = rank
                baseline = pp
        sys.stderr.write('Warning: --baseline not provided, using best guess: "{}"\n'.format(baseline))

    if args.targets:
        targets = args.targets.split(',')
    else:
        targets = list(profile_list)
        targets.remove(baseline)

    common_prefix = os.path.commonprefix(targets).rstrip('_')

    # Test if Epochs were used
    if rrc.get_epoch_df()[:1]['count'].item() > 0.0:
        df = rrc.get_epoch_df()
    else:
        df = rrc.get_app_df()

    result = summary(df=df,
                     perf_metric=args.performance_metric,
                     use_stdev=args.use_stdev,
                     baseline=baseline,
                     targets=targets,
                     show_details=args.show_details)

    title = os.path.commonprefix(targets)
    title = title.rstrip('_')
    title = title + ' (' + args.performance_metric
    if args.use_stdev:
        title += ', stdev)'
    else:
        title += ', min-max)'
    perf_label, energy_label = report.perf_metric_label(args.performance_metric)

    plot_bars(result, baseline, title, perf_label, energy_label,
              xlabel=args.xlabel,
              output_dir=output_dir, show_details=args.show_details)
