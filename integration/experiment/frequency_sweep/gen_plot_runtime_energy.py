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
import pandas
import argparse
import matplotlib.pyplot as plt

import geopmpy.io

from experiment import common_args


def generate_runtime_energy_plot(df, name, output_dir='.'):
    """
    Creates a plot comparing the runtime and energy of a region on two axes.
    """
    f, ax = plt.subplots()

    ax.plot(df.index, df['energy_pkg'], color='DarkBlue', label='Energy', marker='o', linestyle='-')
    ax.set_xlabel('Frequency (GHz)')
    ax.set_ylabel('Energy (J)')

    ax2 = ax.twinx()
    ax2.plot(df.index, df['runtime'], color='Red', label='Runtime', marker='s', linestyle='-')
    ax2.set_ylabel('Runtime (s)')

    lines, labels = ax.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()

    plt.legend(lines + lines2, labels + labels2, shadow=True, fancybox=True, loc='best')
    plt.title('{}'.format(name))
    f.tight_layout()
    plt.savefig(os.path.join(output_dir, '{}_freq_energy.{}'.format(name.lower().replace(' ', '_'), 'png')))
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


def plot_runtime_energy(report_df, label, output_dir, show_details=False):
    # rename some columns
    # TODO: only works for freq map agent
    report_df['freq_hz'] = report_df['FREQ_DEFAULT']
    report_df['runtime'] = report_df['runtime (sec)']
    report_df['network_time'] = report_df['network-time (sec)']
    report_df['energy_pkg'] = report_df['package-energy (joules)']
    report_df['frequency'] = report_df['frequency (Hz)']
    report_df = report_df[['freq_hz', 'runtime', 'region', 'network_time',
                           'energy_pkg', 'frequency', 'count']]

    longest_region = find_longest_region(report_df)
    region_df = report_df.loc[report_df['region'] == longest_region]
    data = []
    freqs = sorted(region_df['freq_hz'].unique())
    for freq in freqs:
        freq_df = region_df.loc[region_df['freq_hz'] == freq]
        region_mean_runtime = freq_df['runtime'].mean()
        region_mean_energy = freq_df['energy_pkg'].mean()
        data.append([region_mean_runtime, region_mean_energy])
    result = pandas.DataFrame(data, index=freqs,
                              columns=['runtime', 'energy_pkg'])

    if show_details:
        sys.stdout.write('Data for longest region "{}":\n{}\n'.format(longest_region, result))

    generate_runtime_energy_plot(result, label, output_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_label(parser)

    args, _ = parser.parse_known_args()
    output_dir = args.output_dir

    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    plot_runtime_energy(output.get_df(),
                        label=args.label,
                        output_dir=output_dir,
                        show_details=args.show_details)
