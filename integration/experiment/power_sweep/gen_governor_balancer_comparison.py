#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Compare a set of Governor and Balancer reports
'''

import argparse
import sys
import os
import warnings

import pandas

import geopmpy.io
from experiment import common_args

# Suppress Pandas PerformanceWarning
warnings.simplefilter(action='ignore', category=pandas.errors.PerformanceWarning)

def prepare(df):
    # Drop unneeded data columns
    extra_cols = ['Start Time', 'GEOPM Version']
    df = df.drop(extra_cols, axis=1)

    # Rename confusing fields
    new_names = {'POWER_PACKAGE_LIMIT_TOTAL' : 'power_limit',
                 'frequency (%)' : 'core freq (%)',
                 'frequency (Hz)' : 'core freq (Hz)'}
    df = df.rename(columns=new_names)

    # Set the index
    df['power_limit'] = df['power_limit'].astype(int)
    df = df.set_index(['Agent', 'power_limit', 'Profile', 'host'])

    # Extract iteration from Profile
    rename_map = {}
    prof_list = df.index.unique('Profile').to_list()
    for prof in prof_list:
        rename_map[prof] = int(prof.split('_')[-1])
    df = df.rename(rename_map)
    df.index = df.index.set_names('iteration', level='Profile')

    # Sort decending by power limit, ascending by iteration
    df = df.sort_index(level=['power_limit', 'iteration'], ascending=[False, True])

    return df

def report_analysis(report_collection, analysis_dir):
    if not os.path.exists(analysis_dir):
        os.mkdir(analysis_dir)

    # Test if Epochs were used
    if report_collection.get_epoch_df()[:1]['count'].item() > 0.0:
        df = report_collection.get_epoch_df()
    else:
        df = report_collection.get_app_df()

    # Drop unneeded cols, rename confusing fields
    df = prepare(df)

    max_limit = df.index.unique('power_limit').tolist()[0]
    node_count = len(df.xs(('power_governor', max_limit, 0)))
    iteration_count = len(df.index.unique('iteration'))

    field_list = ['count', 'runtime (s)', 'sync-runtime (s)', 'time-hint-network (s)', 'package-energy (J)', 'power (W)', 'core freq (%)', 'core freq (Hz)']

    with pandas.option_context('display.max_columns', None, 'display.width', None):
        # Detailed stats
        power_details_log = os.path.join(analysis_dir, 'power_details.log')
        with open(power_details_log, 'w') as log:
            for (power_limit, agent), ldf in df.groupby(['power_limit', 'Agent'], sort=False):
                log.write('-' * 80 + '\n')
                log.write('{} W: Agent = {}\n'.format(power_limit, agent))
                log.write('{}\n'.format(ldf[field_list].describe()))
        sys.stdout.write('Wrote {}\n'.format(power_details_log))

        # Mean stats
        power_means_log = os.path.join(analysis_dir, 'power_means.log')
        with open(power_means_log, 'w') as log:
            log.write('Mean Epoch Statistics for {} nodes over {} iteration(s)\n'.format(node_count, iteration_count))
            results = df.groupby(['Agent', 'power_limit'])[field_list].mean()
            results = results.sort_index(level=['Agent', 'power_limit'], ascending=[False, False])
            log.write('=' * 80 + '\n')
            log.write('\n{}\n\n'.format(results))

            #Calculate runtime improvements
            improvement_fields = ['package-energy (J)', 'sync-runtime (s)']
            a = df.xs(('power_governor'))[improvement_fields].groupby('power_limit').mean()
            b = df.xs(('power_balancer'))[improvement_fields].groupby('power_limit').mean()
            improvement = (a - b) / a
            improvement = improvement.sort_index(ascending=False)
            log.write('Balancer vs. Governor Improvement:\n\n{}\n\n'.format(improvement))
        sys.stdout.write('Wrote {}\n'.format(power_means_log))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_analysis_dir(parser)

    args, _ = parser.parse_known_args()

    try:
        rrc = geopmpy.io.RawReportCollection('*report', dir_name=args.output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a power sweep (ideally with Epoch markup) before using this analysis\n'.format(args.output_dir))
        sys.exit(1)

    report_analysis(rrc, args.analysis_dir)

