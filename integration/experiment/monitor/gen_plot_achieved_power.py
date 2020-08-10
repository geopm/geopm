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
Plot a histogram of the achieved power on each node from an unconstrained run.
'''

import sys
import os
import pandas
import argparse
import matplotlib.pyplot as plt
import numpy

import geopmpy.io

from experiment import common_args
from experiment import machine
# TODO: need common thing
from experiment.power_sweep import gen_node_efficiency


def plot_node_power(df, app_name, min_power, max_power, step_power, output_dir,
                    show_details=False):

    # rename some columns
    df['runtime'] = df['runtime (sec)']
    df['network_time'] = df['network-time (sec)']
    df['energy_pkg'] = df['package-energy (joules)']
    df['frequency'] = df['frequency (Hz)']
    df.set_index(['host'], inplace=True)

    # TODO: per package info
    energy_data = df.groupby(['host']).mean()['energy_pkg'].sort_values()
    runtime_data = df.groupby(['host']).mean()['runtime'].sort_values()
    power_data = energy_data / runtime_data
    power_data = power_data.sort_values()
    power_data = pandas.DataFrame(power_data, columns=['power'])

    if show_details:
        sys.stdout.write('{}\n'.format(power_data))

    min_drop = min_power
    max_drop = max_power - step_power
    bin_size = step_power
    xprecision = 3
    gen_node_efficiency.generate_histogram(power_data, app_name,
                                           min_drop, max_drop, 'power',
                                           bin_size, xprecision, output_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_label(parser)
    common_args.add_show_details(parser)
    # common_args.add_min_power(parser)
    # common_args.add_max_power(parser)

    args, _ = parser.parse_known_args()
    output_dir = args.output_dir

    mach = machine.get_machine(output_dir)
    min_power = mach.power_package_min()
    max_power = mach.power_package_tdp()
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
