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
Print a summary of the runtime and performance metrics for a set of
runs with and without the GEOPM controller on dedicated cores.
'''

import argparse
import sys

import geopmpy.io
import geopmpy.hash
from experiment import common_args


def extract_prefix(name):
    ''' Remove the iteration number after the last underscore. '''
    return '_'.join(name.split('_')[:-1])


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_use_stdev(parser)
    common_args.add_show_details(parser)

    args = parser.parse_args()

    output_dir = args.output_dir
    try:
        rrc = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except RuntimeError as ex:
        sys.stderr.write('<geopm> Error: No report data found in {}: {}\n'.format(output_dir, ex))
        sys.exit(1)

    df = rrc.get_app_df()

    if args.show_details:
        # Raw data from separate trials; condense host rows
        temp = df.set_index('Profile')
        temp = temp.groupby('Profile').mean()
        sys.stdout.write('{}\n'.format(temp[['total_runtime', 'FOM']]))

    # remove iteration from end of profile name
    df['Profile'] = df['Profile'].apply(extract_prefix)
    # Note: FOM and runtime from different nodes of the same run with
    # be the same, so no need to explicitly average before calculating
    # error
    df = df.set_index('Profile')
    group = df.groupby('Profile')
    result = group.mean()
    result = result[['total_runtime', 'FOM']]
    result['min_runtime'] = group['total_runtime'].min()
    result['max_runtime'] = group['total_runtime'].max()
    result['std_runtime'] = group['total_runtime'].std()
    result['min_fom'] = group['FOM'].min()
    result['max_fom'] = group['FOM'].max()
    result['std_fom'] = group['FOM'].std()
    if args.use_stdev:
        sys.stdout.write('Using stdev for error percent\n')
        result['error_pct_runtime'] = result['std_runtime'] / result['total_runtime']
        result['error_pct_fom'] = result['std_fom'] / result['FOM']
    else:
        sys.stdout.write('Using min-max for error percent\n')
        # TODO: what is this supposed to be for min-max
        result['error_pct_runtime'] = (result['max_runtime'] - result['min_runtime']) / result['total_runtime']
        result['error_pct_fom'] = (result['max_fom'] - result['min_fom']) / result['FOM']

    sys.stdout.write('{}\n'.format(result))
