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


# TODO: rename variables/column headers to be consistent
def summary(df, perf_metric, use_stdev, baseline, targets, show_details):

    report.prepare_columns(df, perf_metric)

    result = report.energy_perf_summary(df=df,
                                        loop_key='Profile',
                                        loop_vals=targets,
                                        baseline=baseline,
                                        perf_metric=perf_metric,
                                        use_stdev=use_stdev)

    # reset output stats
    # TODO: make this less of a mess
    output_prefix = os.path.join(output_dir, '{}'.format(common_prefix))
    output_stats_name = '{}_stats.log'.format(output_prefix)
    with open(output_stats_name, 'w') as outfile:
        # re-create file to be appended to
        pass

    if show_details:
        sys.stdout.write('{}\n'.format(result))
    with open(output_stats_name, 'a') as outfile:
        outfile.write('{}\n'.format(result))

    return result


def plot_bars(df, baseline_profile, title, perf_label, xlabel, output_dir, show_details):

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
    ax.bar(points + bar_width / 2, df['energy'], width=bar_width, color='purple',
           label='Energy')
    yerr = (df['min_delta_energy'], df['max_delta_energy'])
    ax.errorbar(points + bar_width / 2, df['energy'], xerr=None,
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
    # TODO: title to filename helper
    fig_name = '{}_bar'.format(title.lower()
                               .replace(' ', '_')
                               .replace(')', '')
                               .replace('(', '')
                               .replace(',', ''))
    fig_name = os.path.join(fig_dir, '{}.png'.format(fig_name))
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
    perf_label = report.perf_metric_label(args.performance_metric)

    plot_bars(result, baseline, title, perf_label,
              xlabel=args.xlabel,
              output_dir=output_dir, show_details=args.show_details)
