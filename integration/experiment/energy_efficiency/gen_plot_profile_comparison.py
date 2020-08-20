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
import pandas
import matplotlib.pyplot as plt

import geopmpy.io
import geopmpy.hash
from experiment import common_args


# TODO: we assume profile name is unique for baseline, but we might want the agent also


def extract_prefix(name):
    ''' Remove the iteration number after the last underscore. '''
    return '_'.join(name.split('_')[:-1])


def summary(df, baseline, targets, show_details):
    # remove iteration from end of profile name
    df['Profile'] = df['Profile'].apply(extract_prefix)

    # rename some columns
    df['energy'] = df['package-energy (joules)']
    df['runtime'] = df['runtime (sec)']

    # choose columns of interest
    df = df[['Profile', 'Agent', 'FREQ_DEFAULT', 'FREQ_UNCORE', 'HASH_0', 'FREQ_0', 'energy', 'runtime']]

    # check that baseline is present
    profiles = df['Profile'].unique()
    if baseline not in profiles:
        raise RuntimeError('Baseline profile prefix "{}" not present in data.'.format(baseline))
    base_data = df.loc[df['Profile'] == baseline]

    base_runtime = base_data['runtime'].mean()
    base_energy = base_data['energy'].mean()
    if show_details:
        sys.stdout.write('Baseline runtime data:\n{}\n\n'.format(base_data['runtime'].describe()))
        sys.stdout.write('Baseline energy data:\n{}\n\n'.format(base_data['energy'].describe()))

    data = []
    xnames = []
    for target_name in targets:
        # check that target is present
        if target_name not in profiles:
            raise RuntimeError('Target profile prefix "{}" not present in data.'.format(target_name))
        target_data = df.loc[df['Profile'] == target_name]

        target_runtime = target_data['runtime'].mean()
        target_energy = target_data['energy'].mean()
        norm_runtime = target_runtime / base_runtime
        norm_energy = target_energy / base_energy
        # for error bars, normalize to same baseline mean
        min_runtime = target_data['runtime'].min() / base_runtime
        max_runtime = target_data['runtime'].max() / base_runtime
        min_energy = target_data['energy'].min() / base_energy
        max_energy = target_data['energy'].max() / base_energy
        std_runtime = target_data['runtime'].std() / base_runtime
        std_energy = target_data['energy'].std() / base_energy
        min_delta_runtime = norm_runtime - min_runtime
        max_delta_runtime = max_runtime - norm_runtime
        min_delta_energy = norm_energy - min_energy
        max_delta_energy = max_energy - norm_energy
        data.append([target_runtime, target_energy,
                     norm_runtime, norm_energy,
                     std_runtime, std_energy,
                     min_delta_runtime, max_delta_runtime,
                     min_delta_energy, max_delta_energy])
        xnames.append(target_name)

    result = pandas.DataFrame(data, index=xnames,
                              columns=['runtime', 'energy',
                                       'runtime_norm', 'energy_norm',
                                       'runtime_std', 'energy_std',
                                       'min_delta_runtime', 'max_delta_runtime',
                                       'min_delta_energy', 'max_delta_energy'])
    return result


def plot_bars(df, baseline_profile, xlabel, output_dir, use_stdev=False):

    labels = df.index.format()
    title = os.path.commonprefix(labels)
    labels = list(map(lambda x: x[len(title):], labels))
    title = title.rstrip('_')
    points = numpy.arange(len(df.index))
    bar_width = 0.35

    errorbar_format = {'fmt': ' ',  # no connecting line
                       'label': '',
                       'color': 'r',
                       'elinewidth': 2,
                       'capthick': 2,
                       'zorder': 10}

    f, ax = plt.subplots()
    # plot runtime
    ax.bar(points - bar_width / 2, df['runtime_norm'], width=bar_width, color='blue',
           label='runtime')
    yerr = (df['min_delta_runtime'], df['max_delta_runtime'])
    if use_stdev:
        yerr = (df['runtime_std'], df['runtime_std'])
    ax.errorbar(points - bar_width / 2, df['runtime_norm'], xerr=None,
                yerr=yerr, **errorbar_format)

    # plot energy
    ax.bar(points + bar_width / 2, df['energy_norm'], width=bar_width, color='cyan',
           label='energy')
    yerr = (df['min_delta_energy'], df['max_delta_energy'])
    if use_stdev:
        yerr = (df['energy_std'], df['energy_std'])
    ax.errorbar(points + bar_width / 2, df['energy_norm'], xerr=None,
                yerr=yerr, **errorbar_format)

    # baseline
    ax.axhline(y=1.0, color='orange', linestyle='dotted')

    ax.set_xticks(points)
    ax.set_xticklabels(labels, rotation='vertical')
    ax.set_xlabel(xlabel)
    f.subplots_adjust(bottom=0.25)  # TODO: use longest profile name
    ax.set_ylabel('Normalized runtime (s) or energy (J)')
    plt.title('{}\nBaseline: {}'.format(title, baseline_profile))
    plt.legend()
    fig_dir = os.path.join(output_dir, 'figures')
    if not os.path.exists(fig_dir):
        os.mkdir(fig_dir)
    plt.savefig(os.path.join(fig_dir, '{}_bar.png'.format(title.lower())))


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)

    parser.add_argument('--baseline', dest='baseline',
                        action='store', default=None,
                        help='baseline profile name')
    parser.add_argument('--targets', dest='targets',
                        action='store', default=None,
                        help='comma-separated list of profile names to compare')
    parser.add_argument('--show-profiles', dest='show_profiles',
                        action='store_true', default=False,
                        help='show all profiles present in the discovered reports')
    parser.add_argument('--xlabel', dest='xlabel',
                        action='store', default='Profile',
                        help='x-axis label for profiles')
    parser.add_argument('--use-stdev', dest='use_stdev',
                        action='store_true', default=False,
                        help='use standard deviation instead of min-max spread for error bars')

    args, extra_cli_args = parser.parse_known_args()

    if (not args.baseline or not args.targets) and not args.show_profiles:
        sys.stderr.write('Profile prefixes for baseline and targets must be provided.  Show all profiles with --show-profiles.\n')
        parser.print_help()
        sys.exit(1)

    output_dir = args.output_dir
    try:
        rrc = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}\n'.format(output_dir))
        sys.exit(1)

    if args.show_profiles:
        profile_list = rrc.get_app_df()['Profile'].unique()
        profile_list = sorted(list(set(map(extract_prefix, profile_list))), reverse=True)
        sys.stdout.write(','.join(profile_list) + '\n')
        sys.exit(0)

    baseline = args.baseline
    targets = args.targets.split(',')
    show_details = args.show_details

    result = summary(rrc.get_app_df(), baseline, targets, show_details)
    if show_details:
        sys.stdout.write('{}\n'.format(result))

    plot_bars(result, baseline, args.xlabel, output_dir, args.use_stdev)
