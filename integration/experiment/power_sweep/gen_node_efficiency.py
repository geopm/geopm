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
import matplotlib.pyplot as plt
import argparse

import geopmpy.io

from experiment import util
from experiment import common_args
from experiment import machine


def generate_histogram(data, app_name, min_drop, max_drop, label, bin_size,
                       xprecision, output_dir):
    data = data[label]
    fontsize = 12
    fig_size = (8, 4)
    verbose = True

    if label.lower() == 'power':
        axis_units = 'W'
        title_units = 'W'
        range_factor = 1
        title = '{}: Histogram of Power (No Capping)'.format(app_name)
        bar_color = 'red'
    elif label.lower() == 'frequency':
        axis_units = 'GHz'
        title_units = 'MHz'
        range_factor = 1000
        title = '{} Histogram of Achieved Frequency'.format(app_name)
        bar_color = 'blue'
    elif label.lower() == 'energy':
        axis_units = 'J'
        title_units = 'J'
        range_factor = 1
        title = '{} Histogram of Energy'.format(app_name)
        bar_color = 'cyan'
    else:
        raise RuntimeError("<geopmpy>: Unknown type for histogram: {}".format(label))

    plt.figure(figsize=fig_size)
    bins = [round(bb*bin_size, 3) for bb in range(int(min_drop/bin_size), int(max_drop/bin_size)+2)]
    n, bins, patches = plt.hist(data, rwidth=0.8, bins=bins, color=bar_color)
    for n, b in zip(n, bins):
        plt.annotate(int(n) if int(n) != 0 else "", xy=(b+bin_size/2.0, n+2.5),
                     horizontalalignment='center',
                     fontsize=fontsize-4)
    min_max_range = (max(data) - min(data)) * range_factor
    mean = data.mean() * range_factor

    n = len(data)
    trim_pct = 0.05
    trimmed_data = data[int(n*trim_pct):n-int(trim_pct*n)]
    trimmed_min_max = (max(trimmed_data) - min(trimmed_data)) * range_factor
    plt.title('{}\nMin-max Var.: {} {}; {}% Min-max Var.: {} {}; Mean: {} {}'
              .format(title, round(min_max_range, 3), title_units,
                      int((trim_pct)*100), round(trimmed_min_max, 3), title_units,
                      round(mean, 3), title_units),
              fontsize=fontsize)
    plt.xlabel('{} ({})'.format(label.title(), axis_units), fontsize=fontsize)
    plt.ylabel('Count', fontsize=fontsize)
    plt.xticks([b+bin_size/2.0 for b in bins],
               [' [{start:.{prec}f}, {end:.{prec}f})'.format(start=b, end=b+bin_size, prec=xprecision) for b in bins],
               rotation='vertical',
               fontsize=fontsize-4)
    _, ylabels = plt.yticks()
    plt.setp(ylabels, fontsize=fontsize-4)

    plt.margins(0.02, 0.2)
    plt.axis('tight')

    plt.tight_layout()
    fig_dir = os.path.join(output_dir, 'figures')
    if not os.path.exists(fig_dir):
        os.makedirs(fig_dir)

    filename = '{}_{}_histo'.format(app_name.lower().replace('@', '_').replace(' ', '_'), label)
    # todo: could be a method
    for ext in ['png']:
        full_path = os.path.join(fig_dir, '{}.{}'.format(filename, ext))
        plt.savefig(full_path)
        if verbose:
            sys.stdout.write('    {}\n'.format(full_path))
    plt.close()


def achieved_freq_histogram_package(app_name, output_dir, report_df, detailed=False):
    # min and max are used to create consistent x-axis limits
    # sticker frequency is used when converting the percent-of-sticker to
    # a value in hertz
    machine = util.get_machine(output_dir)
    min_freq = machine.frequency_min()
    max_freq = machine.frequency_max()
    sticker_freq = machine.frequency_sticker()
    step_freq = machine.frequency_step()

    report_df['power_limit'] = report_df['POWER_PACKAGE_LIMIT_TOTAL']

    temp_df = report_df.copy()
    report_df['frequency'] = report_df['CYCLES_THREAD@package-0'] / report_df['CYCLES_REFERENCE@package-0']
    temp_df['frequency'] = temp_df['CYCLES_THREAD@package-1'] / temp_df['CYCLES_REFERENCE@package-1']
    report_df['freq_package'] = 0
    temp_df['freq_package'] = 1
    report_df.set_index(['power_limit', 'host', 'freq_package'], inplace=True)
    temp_df.set_index(['power_limit', 'host', 'freq_package'], inplace=True)
    report_df = report_df.append(temp_df)
    # convert percent to GHz frequency based on sticker
    report_df['frequency'] *= sticker_freq / 1e9

    profiles = report_df['POWER_PACKAGE_LIMIT_TOTAL'].unique()
    power_caps = sorted(profiles)  # list(range(self._min_power, self._max_power+1, self._step_power))
    gov_freq_data = {}
    bal_freq_data = {}
    for target_power in power_caps:
        governor_data = report_df.loc[report_df["Agent"] == "power_governor"]
        governor_data = governor_data.loc[governor_data['POWER_PACKAGE_LIMIT_TOTAL'] == target_power]
        gov_freq_data[target_power] = governor_data.groupby(['host', 'freq_package']).mean()['frequency'].sort_values()
        gov_freq_data[target_power] = pandas.DataFrame(gov_freq_data[target_power])
        if detailed:
            sys.stdout.write('Governor data @ {}W:\n{}\n'.format(target_power, gov_freq_data[target_power]))

        balancer_data = report_df.loc[report_df["Agent"] == "power_balancer"]
        balancer_data = balancer_data.loc[balancer_data['POWER_PACKAGE_LIMIT_TOTAL'] == target_power]
        bal_freq_data[target_power] = balancer_data.groupby(['host', 'freq_package']).mean()['frequency'].sort_values()
        bal_freq_data[target_power] = pandas.DataFrame(bal_freq_data[target_power])
        if detailed:
            sys.stdout.write('Balancer data @ {}W:\n{}\n'.format(target_power, bal_freq_data[target_power]))

    # plot histograms
    min_drop = min_freq / 1e9
    max_drop = (sticker_freq + step_freq) / 1e9
    #max_drop = (max_freq - step_freq) / 1e9  # turbo range
    bin_size = step_freq / 1e9 / 2.0
    for target_power in power_caps:
        gov_data = gov_freq_data[target_power]
        bal_data = bal_freq_data[target_power]

        name = app_name + "@" + str(target_power) + "W Governor"
        generate_histogram(gov_data, name, min_drop, max_drop, 'frequency',
                           bin_size, 3, output_dir)
        name = app_name + "@" + str(target_power) + "W Balancer"
        generate_histogram(bal_data, name, min_drop, max_drop, 'frequency',
                           bin_size, 3, output_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    parser.add_argument('--label', action='store', default="APP",
                        help='name of the application to use for plot titles')

    args, _ = parser.parse_known_args()
    output_dir = args.output_dir
    show_details = args.show_details
    label = args.label
    output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    achieved_freq_histogram_package(app_name=label,
                                    output_dir=output_dir,
                                    report_df=output.get_epoch_df(),
                                    detailed=show_details)
