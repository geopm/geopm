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
Prints a summary of the data from a power sweep experiment.
'''

import sys
import pandas
import argparse

import geopmpy.io

from experiment import common_args


def summary(parse_output):
    # rename some columns
    parse_output['power_limit'] = parse_output['POWER_PACKAGE_LIMIT_TOTAL']
    parse_output['runtime'] = parse_output['runtime (s)']
    parse_output['network_time'] = parse_output['time-hint-network (s)']
    parse_output['energy_pkg'] = parse_output['package-energy (J)']
    parse_output['energy_dram'] = parse_output['dram-energy (J)']
    parse_output['frequency'] = parse_output['frequency (Hz)']
    parse_output['achieved_power'] = parse_output['energy_pkg'] / parse_output['sync-runtime (s)']
    parse_output['iteration'] = parse_output.apply(lambda row: row['Profile'].split('_')[-1],
                                                   axis=1)
    # add extra columns
    parse_output['cpu_time'] = parse_output['runtime'] - parse_output['network_time']

    # set up index for grouping
    parse_output = parse_output.set_index(['Agent', 'host', 'power_limit'])
    summary = pandas.DataFrame()
    for col in ['count', 'runtime', 'cpu_time', 'network_time', 'energy_pkg', 'energy_dram', 'frequency', 'achieved_power']:
        summary[col] = parse_output[col].groupby(['Agent', 'power_limit']).mean()
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    args = parser.parse_args()
    output_dir = args.output_dir

    output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    result = summary(output.get_epoch_df())

    sys.stdout.write('{}\n'.format(result))
