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
Creates a plot of the runtime and energy of a region of interest
versus the fixed frequency from a frequency sweep.
'''

import sys
import os
import argparse
import matplotlib.pyplot as plt

import geopmpy.io

from experiment import common_args
from experiment import report


def generate_runtime_energy_plot(df, perf_metric_label, title, output_dir='.'):
    """
    Creates a plot comparing the runtime and energy of a region on two axes.
    """
    errorbar_format = {'fmt': ' ',
                       'label': '',
                       'elinewidth': 1,
                       'zorder': 10}

    f, ax = plt.subplots()
    points = df.index

    ax.plot(points, df['energy'], color='purple', label='Energy', marker='o', linestyle='-')
    ax.set_ylabel('Energy (J)')
    yerr = (df['min_delta_energy'], df['max_delta_energy'])
    ax.errorbar(points, df['energy'], xerr=None,
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
    figname = '{}_freq_energy'.format(title.lower().replace(' ', '_').replace('(', '').replace(')', ''))
    plt.savefig(os.path.join(output_dir, 'figures', figname))
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

    report.prepare_columns(report_df, perf_metric)
    #report_df = report_df[['FREQ_DEFAULT', 'runtime', 'network_time', 'energy', 'FOM']]
    # TODO: might want loop_key to be passed in as a function instead
    freqs = sorted(report_df['FREQ_DEFAULT'].unique())
    result = report.energy_perf_summary(df=report_df,
                                        loop_key='FREQ_DEFAULT',
                                        loop_vals=freqs,
                                        baseline=None,
                                        perf_metric=perf_metric,
                                        use_stdev=use_stdev)
    perf_metric_label = report.perf_metric_label(perf_metric)

    if show_details:
        sys.stdout.write('Data for {}:\n{}\n'.format(label, result))

    title = label
    if use_stdev:
        title += ' (stdev)'
    else:
        title += ' (min-max)'
    generate_runtime_energy_plot(result, perf_metric_label, title, output_dir)


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
        data = output.get_app_df()

    label += ' ' + args.performance_metric

    plot_runtime_energy(report_df=data,
                        use_stdev=args.use_stdev,
                        label=label,
                        output_dir=output_dir,
                        show_details=args.show_details,
                        perf_metric=args.performance_metric)
