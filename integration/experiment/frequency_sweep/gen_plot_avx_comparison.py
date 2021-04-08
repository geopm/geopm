#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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
Compares 2 sets of data based on the presense of 'avx2' or 'avx512' in the Profile string
'''

import sys
import os
import pandas
import argparse
import matplotlib.pyplot as plt
import numpy

import geopmpy.io

from experiment import common_args

pandas.set_option('display.width', None)
pandas.set_option('display.max_columns', None)
pandas.set_option('display.max_rows', None)


def prepare(df):
    # Drop unneeded data columns
    extra_cols = ['FREQ_{}'.format(ii) for ii in range(0,31)]
    extra_cols += ['HASH_{}'.format(ii) for ii in range(0,31)]
    extra_cols += ['Start Time', 'GEOPM Version']
    df = df.drop(extra_cols, axis=1)

    # Rename confusing fields
    new_names = {"FREQ_DEFAULT" : "core_mhz",
                 "FREQ_UNCORE" : "uncore_mhz",
                 "frequency (%)" : "core freq (%)",
                 "frequency (Hz)" : "core freq (Hz)"}
    df = df.rename(columns=new_names)

    # Convert frequency fields for index to MHz
    hz_to_mhz = lambda x : int(x / 1e6)
    df['core_mhz'] = df['core_mhz'].apply(hz_to_mhz)

    return df

def plot_profile_comparison(report_collection, args):
    metric = {'runtime'   : 'runtime (s)',
              'energy'    : 'package-energy (J)',
              'power'     : 'power (W)',
              'frequency' : 'core freq (%)'}.get(args.metric)

    df = report_collection.get_df()
    adf = report_collection.get_app_df()
    edf = report_collection.get_epoch_df()

    # Drop unneeded cols, rename confusing fields, convert index fields to MHz
    df = prepare(df)
    edf = prepare(edf)
    adf = prepare(adf)

    adf_field_list = ['runtime (s)', 'sync-runtime (s)', 'time-hint-network (s)',
                      'package-energy (J)', 'power (W)', 'core freq (%)', 'core freq (Hz)']
    field_list = ['count'] + adf_field_list # For regions and the Epoch data

    # Check the profile string to determine which avx flag was used
    avx_flag = lambda ps : 'avx2' if 'avx2' in ps else ('avx512' if 'avx512' in ps else 'n/a')
    adf['avx_flag'] = adf['Profile'].apply(avx_flag)

    # Write raw data
    with open(os.path.join(args.output_dir, args.analysis_dir, 'raw_stats.log'), 'w') as log:
        gadf = adf.groupby(['avx_flag', 'core_mhz'])
        results = gadf[adf_field_list].mean()

        log.write('App totals, means (all nodes/iterations)\n')
        log.write('{}\n\n'.format(results))
        log.write('=' * 100 + '\n\n')

        log.write('App totals, full stats (all nodes/iterations)\n')
        for ff in adf_field_list:
            results = gadf[ff].describe()

            log.write('-' * 100 + '\n\n')
            log.write('Metric - {}\n'.format(ff))
            log.write('{}\n\n'.format(results))

    # Below this point is setup for the plotting
    #   Based on: power_sweep/gen_plot_balancer_comparison.py

    # Create a groupby object to identify the unique datasets for analysis
    gadf = adf.groupby(['avx_flag'])

    reference_group = gadf.get_group('avx2').groupby('core_mhz')
    target_group = gadf.get_group('avx512').groupby('core_mhz')

    # Setup plot_df for bar charts, also performs data reduction if multiple iterations
    plot_df = pandas.DataFrame()
    plot_df['reference_mean'] = reference_group[metric].mean()
    plot_df['reference_max'] = reference_group[metric].max()
    plot_df['reference_min'] = reference_group[metric].min()
    plot_df['target_mean'] = target_group[metric].mean()
    plot_df['target_max'] = target_group[metric].max()
    plot_df['target_min'] = target_group[metric].min()

    # Convert the maxes and mins to be deltas from the mean; required for the errorbar API
    plot_df['reference_max_delta'] = plot_df['reference_max'] - plot_df['reference_mean']
    plot_df['reference_min_delta'] = plot_df['reference_mean'] - plot_df['reference_min']
    plot_df['target_max_delta'] = plot_df['target_max'] - plot_df['target_mean']
    plot_df['target_min_delta'] = plot_df['target_mean'] - plot_df['target_min']

    f, ax = plt.subplots()
    bar_width = 0.35
    index = numpy.arange(min(len(plot_df['target_mean']), len(plot_df['reference_mean'])))

    plt.bar(index - bar_width / 2,
            plot_df['reference_mean'],
            width=bar_width,
            color='blue',
            align='center',
            label='avx2',
            zorder=3)

    ax.errorbar(index - bar_width / 2,
                plot_df['reference_mean'],
                xerr=None,
                yerr=(plot_df['reference_min_delta'], plot_df['reference_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    plt.bar(index + bar_width / 2,
            plot_df['target_mean'],
            width=bar_width,
            color='cyan',
            align='center',
            label='avx512',
            zorder=3)  # Forces grid lines to be drawn behind the bar

    ax.errorbar(index + bar_width / 2,
                plot_df['target_mean'],
                xerr=None,
                yerr=(plot_df['target_min_delta'], plot_df['target_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    ax.set_xticks(index)
    xlabels = [int(xx) for xx in plot_df.index]
    ax.set_xticklabels(xlabels)
    ax.set_xlabel('Requested Core Frequency (MHz)')

    ax.set_ylabel(metric.capitalize())

    ax.grid(axis='y', linestyle='--', color='black')

    plt.title('{} : AVX Flag Comparison'.format(args.label.title()), y=1.02)

    plt.margins(0.02, 0.01)
    plt.axis('tight')
    legend_fontsize = 14
    plt.legend(shadow=True, fancybox=True, fontsize=legend_fontsize, loc='lower right').set_zorder(11)
    plt.tight_layout()

    ymax = ax.get_ylim()[1]
    ymax *= 1.1
    ax.set_ylim(0, ymax)

    # Write data/plot files
    file_name = '{}_{}_avx_comparison'.format(args.label, args.metric)

    analysis_dir = os.path.join(args.output_dir, args.analysis_dir)
    if not os.path.exists(analysis_dir):
        os.mkdir(analysis_dir)

    full_path = os.path.join(analysis_dir, file_name)
    plt.savefig(full_path + '.png')
    sys.stdout.flush()
    plt.close()

    #  import code
    #  code.interact(local=dict(globals(), **locals()))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_analysis_dir(parser)
    common_args.add_label(parser)
    parser.add_argument('--metric', action='store', default='runtime',
                        help='metric to use for comparison.  One of: runtime, energy, frequency, power') # For these plots, energy means total socket energy

    args = parser.parse_args()

    try:
        rrc = geopmpy.io.RawReportCollection("*report", dir_name=args.output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    plot_profile_comparison(rrc, args)

