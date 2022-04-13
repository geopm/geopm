#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import pandas
import sys
import os

import geopmpy.io
from experiment import common_args

pandas.set_option('display.width', 250)
pandas.set_option('display.max_colwidth', 30)
pandas.set_option('display.max_rows', None)

def prepare(df):
    # Drop unneeded data columns
    extra_cols = ['Start Time', 'GEOPM Version']
    df = df.drop(extra_cols, axis=1)

    # Rename confusing fields
    new_names = {'POWER_PACKAGE_LIMIT_TOTAL' : 'power_limit',
                 'frequency (%)' : 'core freq (%)',
                 'frequency (Hz)' : 'core freq (Hz)'}
    df = df.rename(columns=new_names)

    df = df.sort_values(['power_limit'], ascending=False)

    return df


def governor_analysis(report_collection, analysis_dir):
    if not os.path.exists(analysis_dir):
        os.mkdir(analysis_dir)

    edf = report_collection.get_epoch_df()
    edf = prepare(edf) # Drop unneeded cols, rename confusing fields

    field_list = ['count', 'runtime (s)', 'sync-runtime (s)', 'time-hint-network (s)', 'package-energy (J)', 'power (W)', 'core freq (%)', 'core freq (Hz)']

    gedf = edf.groupby(['Agent', 'power_limit'])
    means_edf = gedf[field_list].mean()
    means_edf = means_edf.sort_index(level=['Agent', 'power_limit'], ascending=False)

    with open(os.path.join(analysis_dir, 'gov_mean_stats.log'), 'w') as log:
        results = means_edf.xs('power_governor')
        results = results.copy() # Avoid SettingWithCopyWarning
        results['runtime_degradation (%)'] = (results['runtime (s)'] - results.iloc[0]['runtime (s)']) / results.iloc[0]['runtime (s)']
        results['energy_improvement (%)'] = (results.iloc[0]['package-energy (J)'] - results['package-energy (J)']) / results.iloc[0]['package-energy (J)']

        log.write('Governor Comparison: TDP vs. lower budgets\n')
        log.write('{}\n\n'.format(results))
        log.write('=' * 100 + '\n\n')

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

    governor_analysis(rrc, args.analysis_dir)

