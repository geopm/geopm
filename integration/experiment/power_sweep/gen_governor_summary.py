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

