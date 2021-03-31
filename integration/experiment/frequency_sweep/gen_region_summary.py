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
Generates per-region summaries of the report data.
'''

import sys
import os
import pandas
import argparse

import geopmpy.io

from experiment import common_args

pandas.set_option('display.width', 200)


def region_summary(report_collection, output_dir, show_details):
    df = report_collection.get_df()
    adf = report_collection.get_app_df()
    edf = report_collection.get_epoch_df()

    mhz = lambda x : int(x / 1e6)
    df['FREQ_DEFAULT'] = df['FREQ_DEFAULT'].apply(mhz)  # Convert frequency to MHz for readability
    edf['FREQ_DEFAULT'] = edf['FREQ_DEFAULT'].apply(mhz)
    adf['FREQ_DEFAULT'] = adf['FREQ_DEFAULT'].apply(mhz)

    df = df.set_index(['region', 'FREQ_DEFAULT'])
    df.index = df.index.set_names('freq_mhz', level='FREQ_DEFAULT')
    edf = edf.set_index(['FREQ_DEFAULT'])
    edf.index = edf.index.set_names('freq_mhz')
    adf = adf.set_index(['FREQ_DEFAULT'])
    adf.index = adf.index.set_names('freq_mhz')

    field_list = ['count', 'frequency (%)', 'frequency (Hz)', 'runtime (sec)',
                  'sync-runtime (sec)', 'network-time (sec)', 'package-energy (joules)', 'power (watts)']
    adf_field_list = ['runtime (sec)', 'network-time (sec)', 'package-energy (joules)', 'power (watts)']

    with open(os.path.join(output_dir, 'region_stats.log'), 'w') as log:
        log.write('Per-region (all nodes/iterations)\n')
        for (region, freq), ldf in df.groupby(['region', 'freq_mhz']):
            log.write('-' * 100 + '\n\n')
            log.write('Region: {} | Requested Frequency MHz: {}\n\n'.format(region, freq))
            log.write('{}\n\n'.format(ldf[field_list].describe()))
        log.write('=' * 100 + '\n\n')
        for (freq), ldf in edf.groupby('freq_mhz'):
            log.write('-' * 100 + '\n\n')
            log.write('Region: Epoch | Requested Frequency MHz: {}\n\n'.format(freq))
            log.write('{}\n\n'.format(ldf[field_list].describe()))
        log.write('=' * 100 + '\n\n')
        for (freq), ldf in adf.groupby('freq_mhz'):
            log.write('-' * 100 + '\n\n')
            log.write('Application totals | Requested Frequency MHz: {}\n\n'.format(freq))
            log.write('{}\n\n'.format(ldf[adf_field_list].describe()))
        if show_details:
            sys.stdout.write('Wrote to {}\n'.format(os.path.join(output_dir, 'region_stats.log')))

    # means_df aggregates the desired data across nodes and iterations
    means_df = df.groupby(['region', 'freq_mhz'])[field_list].mean()
    means_edf = edf.groupby(['freq_mhz'])[field_list].mean()
    means_adf = adf.groupby(['freq_mhz'])[adf_field_list].mean()

    # Sort first by region alpha order, then decending by frequency
    means_df = means_df.sort_index(level=['region', 'freq_mhz'], ascending=[True, False])
    means_edf = means_edf.sort_index(level='freq_mhz', ascending=False)
    means_adf = means_adf.sort_index(level='freq_mhz', ascending=False)

    with open(os.path.join(output_dir, 'region_mean_stats.log'), 'w') as log:
        log.write('Per-region means (all nodes/iterations)\n')
        for (region), ldf in means_df.groupby('region'):
            log.write('-' * 100 + '\n\n')
            log.write('Region: {}\n\n'.format(region))
            log.write('{}\n\n'.format(ldf.reset_index(level='region', drop=True)))
        log.write('=' * 100 + '\n\n')
        log.write('Epoch means (all nodes/iterations)\n\n')
        log.write('{}\n\n'.format(means_edf))
        log.write('=' * 100 + '\n\n')
        log.write('Application totals means (all nodes/iterations)\n\n')
        log.write('{}\n\n'.format(means_adf))
        if show_details:
            sys.stdout.write('Wrote to {}\n'.format(os.path.join(output_dir, 'region_mean_stats.log')))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)
    common_args.add_label(parser)

    args = parser.parse_args()

    output_dir = args.output_dir
    try:
        rrc = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    region_summary(rrc, output_dir, args.show_details)
