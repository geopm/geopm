#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
