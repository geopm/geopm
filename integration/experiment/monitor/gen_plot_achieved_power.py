#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Plot a histogram of the achieved power on each package from an unconstrained run.
'''

import sys
import pandas
import argparse

import geopmpy.io

from experiment import common_args
from experiment import machine
# TODO: need common thing
from experiment.power_sweep import gen_plot_node_efficiency


def plot_node_power(df, app_name, min_power, max_power, step_power, output_dir,
                    show_details=False):

    temp_df = df.copy()
    df['package'] = 0
    df['power'] = df['CPU_ENERGY@package-0'] / df['TIME@package-0']
    df.set_index(['host', 'package'], inplace=True)
    temp_df['package'] = 1
    temp_df['power'] = temp_df['CPU_ENERGY@package-1'] / temp_df['TIME@package-1']
    temp_df.set_index(['host', 'package'], inplace=True)

    df = df.append(temp_df)

    power_data = df.groupby(['host', 'package']).mean()['power']
    power_data = power_data.sort_values()
    power_data = pandas.DataFrame(power_data, columns=['power'])

    if show_details:
        sys.stdout.write('{}\n'.format(power_data))

    min_drop = min_power
    max_drop = max_power - step_power
    bin_size = step_power
    xprecision = 3  # TODO: move into function
    gen_plot_node_efficiency.generate_histogram(power_data, app_name,
                                                min_drop, max_drop, 'power',
                                                bin_size, xprecision, output_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_label(parser)
    common_args.add_show_details(parser)

    args = parser.parse_args()
    output_dir = args.output_dir

    mach = machine.get_machine(output_dir)
    # TODO: use per-values; machine needs num packages
    num_package = 2
    min_power = mach.power_package_min() / num_package
    max_power = mach.power_package_tdp() / num_package
    step_power = 10

    # TODO: make into utility function
    try:
        output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)
    except:
        sys.stderr.write('<geopm> Error: No report data found in {}; run a monitor experiment before using this analysis\n'.format(output_dir))
        sys.exit(1)

    plot_node_power(output.get_epoch_df(),
                    app_name=args.label,
                    min_power=min_power,
                    max_power=max_power,
                    step_power=step_power,
                    output_dir=output_dir,
                    show_details=args.show_details)
