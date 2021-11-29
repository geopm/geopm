#!/usr/bin/env python3
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

import argparse
import pandas
import sys
import os
import math

import geopmpy.io
from experiment import common_args

pandas.set_option('display.width', 250)
pandas.set_option('display.max_colwidth', 30)
pandas.set_option('display.max_rows', None)

def prepare(df):
    # Drop unneeded data columns
    extra_cols = ['Start Time', 'GEOPM Version']
    df = df.drop(extra_cols, axis=1)
    df['gpu freq (Hz)'] = df.filter(regex='NVML::FREQUENCY@board_accelerator').mean(1)
    df['gpu power (W)'] = df.filter(regex='NVML::POWER@board_accelerator').sum(1)
    df['gpu energy (J)'] = df.filter(regex='NVML::TOTAL_ENERGY_CONSUMPTION@board_accelerator').sum(1)
    df['uncore freq (Hz)'] = df.filter(regex='MSR::UNCORE_PERF_STATUS:FREQ@package').mean(1)
    
    # Rename confusing fields
    new_names = {"CORE_FREQUENCY" : "core_mhz",
                 "UNCORE_MIN_FREQUENCY" : "uncore_mhz",
                 "ACCELERATOR_FREQUENCY" : "gpu_mhz",
                 "frequency (%)" : "core freq (%)",
                 "frequency (Hz)" : "core freq (Hz)"}
    df = df.rename(columns=new_names)

    # Convert frequency fields for index to MHz
    hz_to_mhz = lambda x : 'NAN' if math.isnan(x) else int(x / 1e6)
    df['core_mhz'] = df['core_mhz'].apply(hz_to_mhz)
    df['uncore_mhz'] = df['uncore_mhz'].apply(hz_to_mhz)
    df['gpu_mhz'] = df['gpu_mhz'].apply(hz_to_mhz)
    df = df.sort_values(['gpu_mhz','core_mhz', 'uncore_mhz'], ascending=False)

    return df

def region_summary_analysis(report_collection, analysis_dir):
    if not os.path.exists(analysis_dir):
        os.mkdir(analysis_dir)

    df = report_collection.get_df()
    edf = report_collection.get_epoch_df()
    adf = report_collection.get_app_df()

    # Drop unneeded cols, rename confusing fields, convert index fields to MHz
    edf = prepare(edf)
    df = prepare(df)
    adf = prepare(adf)

    field_list = ['count', 'runtime (s)', 'sync-runtime (s)', 'time-hint-network (s)', 'package-energy (J)', 'power (W)', 'core freq (%)', 'core freq (Hz)', 'gpu freq (Hz)', 'gpu power (W)', 'gpu energy (J)', 'uncore freq (Hz)']
    adf_field_list = ['runtime (s)', 'time-hint-network (s)', 'time-hint-ignore (s)', 'package-energy (J)', 'dram-energy (J)', 'power (W)', 'gpu power (W)', 'gpu energy (J)']

    if edf[:1]['count'].item() > 0.0:
        # Write Epoch stats
        with open(os.path.join(analysis_dir, 'epoch_mean_stats.log'), 'w') as log:
            gedf = edf.groupby(['gpu_mhz', 'core_mhz', 'uncore_mhz'], sort=False)
            results = gedf[field_list].mean()

            #log.write('Epochs (all nodes/iterations) - default Uncore freq. = 2.4 GHz\n')
            #log.write('{}\n\n'.format(results.xs(2400, level=1)))
            #log.write('=' * 100 + '\n\n')
            log.write('Epochs (all nodes/iterations)\n')
            log.write('{}\n\n'.format(results))

        # Write detailed Epoch stats
        with open(os.path.join(analysis_dir, 'epoch_detailed_stats.log'), 'w') as log:
            log.write('Epochs (all nodes/iterations)\n')
            for (gpu_freq, core_freq, uncore_freq), ledf in edf.groupby(['gpu_mhz', 'core_mhz', 'uncore_mhz'], sort=False):
                log.write('-' * 100 + '\n\n')
                log.write('Requested Accelerator Frequency MHz: {} | Core Frequency MHz: {} | Uncore Frequency MHz: {}\n\n'.format(gpu,freq,core_freq, uncore_freq))
                log.write('{}\n\n'.format(ledf[field_list].describe()))
            log.write('=' * 100 + '\n\n')

    # Write App Totals
    with open(os.path.join(analysis_dir, 'app_totals_mean_stats.log'), 'w') as log:
        gadf = adf.groupby(['gpu_mhz','core_mhz', 'uncore_mhz'], sort=False)
        results = gadf[adf_field_list].mean()

        #log.write('App Totals (all nodes/iterations) - default Uncore freq. = 2.4 GHz\n')
        #log.write('{}\n\n'.format(results.xs(2400, level=1)))
        #log.write('=' * 100 + '\n\n')
        log.write('App Totals (all nodes/iterations)\n')
        log.write('{}\n\n'.format(results))

    # Write detailed App Totals stats
    with open(os.path.join(analysis_dir, 'app_totals_detailed_stats.log'), 'w') as log:
        log.write('App Totals (all nodes/iterations)\n')
        for (gpu_freq, core_freq, uncore_freq), ladf in adf.groupby(['gpu_mhz','core_mhz', 'uncore_mhz'], sort=False):
            log.write('-' * 100 + '\n\n')
            log.write('Requested Accelerator Frequency MHz: {} | Core Frequency MHz: {} | Uncore Frequency MHz: {}\n\n'.format(gpu_freq, core_freq, uncore_freq))
            log.write('{}\n\n'.format(ladf[adf_field_list].describe()))
        log.write('=' * 100 + '\n\n')

    # Write Region stats
    with open(os.path.join(analysis_dir, 'region_mean_stats.log'), 'w') as log:
        gdf = df.groupby(['region', 'gpu_mhz', 'core_mhz', 'uncore_mhz'])
        results = gdf[field_list].mean().sort_index(level=['region', 'gpu_mhz', 'core_mhz', 'uncore_mhz'], ascending=[True, False, False, False]) # Sort these properly before doing anything

        #log.write('Per-region (all nodes/iterations) - default Uncore freq. = 2.4 GHz\n')
        #for (region), ldf in results.groupby(['region'], sort=False):
        #    ldf = ldf.reset_index('region', drop=True)
        #    log.write('-' * 100 + '\n\n')
        #    log.write('Region: {}\n\n'.format(region))
        #    log.write('{}\n\n'.format(ldf.xs(2400, level=1)))

        #log.write('=' * 100 + '\n\n')

        log.write('Per-region (all nodes/iterations)\n')
        for (region), ldf in results.groupby(['region'], sort=False):
            ldf = ldf.reset_index('region', drop=True)
            log.write('-' * 100 + '\n\n')
            log.write('Region: {}\n\n'.format(region))
            log.write('{}\n\n'.format(ldf))

    # Write detailed per-region stats
    sys.stderr.write('Writing detailed per-region stats.  This may take a minute... ')
    with open(os.path.join(analysis_dir, 'region_detailed_stats.log'), 'w') as log:
        log.write('Per-region (all nodes/iterations)\n')
        for (region, gpu_freq, core_freq, uncore_freq), ldf in df.groupby(['region', 'gpu_mhz', 'core_mhz', 'uncore_mhz'], sort=False):
            log.write('-' * 100 + '\n\n')
            log.write('Region: {} | Requested Accelerator Frequency MHz: {} | Core Frequency MHz: {} | Uncore Frequency MHz: {}\n\n'.format(region, gpu_freq, core_freq, uncore_freq))
            log.write('{}\n\n'.format(ldf[field_list].describe()))
        log.write('=' * 100 + '\n\n')
    sys.stderr.write("DONE.\n")

def region_length_analysis(report_collection, analysis_dir):
    if not os.path.exists(analysis_dir):
        os.mkdir(analysis_dir)

    df = report_collection.get_df()
    edf = report_collection.get_epoch_df() # For iteration timing

    # Drop unneeded cols, rename confusing fields, convert index fields to MHz
    df = prepare(df)
    edf = prepare(edf)

    df['runtime-per-iteration'] = df['runtime (s)'] / df['count']
    df['sync-runtime-per-iteration'] = df['sync-runtime (s)'] / df['count']

    field_list = ['runtime-per-iteration', 'runtime (s)', 'sync-runtime-per-iteration', 'sync-runtime (s)', 'time-hint-network (s)', 'count']

    gdf = df.groupby(['region', 'gpu_mhz', 'core_mhz', 'uncore_mhz'], sort=False)
    results = gdf[field_list].mean().sort_index(level=['region', 'gpu_mhz', 'core_mhz', 'uncore_mhz'], ascending=[True, False, False, False])

    gedf = edf.groupby(['gpu_mhz', 'core_mhz', 'uncore_mhz'], sort=False)
    epoch_means = gedf['runtime (s)'].mean().sort_index(level=['gpu_mhz', 'core_mhz', 'uncore_mhz'], ascending=True) # Unclear why True is necessary

    with open(os.path.join(analysis_dir, 'region_iteration_runtime.log'), 'w') as log:
        log.write('Per-region means (all nodes)\n')
        log.write('-' * 80 + '\n')
        for (region), ldf in results.groupby('region'):

            ldf = ldf.copy() # Avoid SettingWithCopyWarning
            ldf = ldf.reset_index('region', drop=True)
            ldf['% of epoch'] = (ldf['runtime (s)'] / epoch_means) * 100
            ldf['epoch_time'] = epoch_means

            log.write('Region = {}\n'.format(region))
            log.write('{}\n\n'.format(ldf))
            log.write('=' * 100 + '\n')

    with open(os.path.join(analysis_dir, 'region_iteration_runtime_filtered.log'), 'w') as log:
        log.write('Per-region means (all nodes) > 5 ms per iteration && total runtime >= 10.0\n')
        log.write('-' * 80 + '\n')
        condition = (results['runtime-per-iteration'] > 0.005) & (results['runtime (s)'] > 10.0)
        for (region), ldf in results.loc[condition].groupby('region'):
            log.write('{}\n\n'.format(ldf))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_analysis_dir(parser)

    args, _ = parser.parse_known_args()

    try:
        rrc = geopmpy.io.RawReportCollection("*report", dir_name=args.output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a core/uncore frequency sweep before using this analysis\n'.format(output_dir))
        sys.exit(1)

    region_summary_analysis(rrc, args.analysis_dir)
    region_length_analysis(rrc, args.analysis_dir)

