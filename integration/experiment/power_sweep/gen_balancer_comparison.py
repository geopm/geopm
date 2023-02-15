#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Example power sweep experiment using geopmbench.
'''

import sys
import pandas
import argparse

import geopmpy.io

from experiment import common_args


def balancer_comparison(report_epoch_data, detailed=False):
    margin_factor = 0.25
    power_caps = report_epoch_data['CPU_POWER_LIMIT'].dropna().unique() # dropna handles monitor runs

    result = pandas.DataFrame(columns=['max_runtime_gov', 'max_runtime_bal', 'margin', 'runtime_pct'])
    for cap in power_caps:
        edf = report_epoch_data
        edf = edf.set_index(['Agent', 'CPU_POWER_LIMIT', 'Profile', 'host'])
        if detailed:
            sys.stdout.write('{}\n'.format(edf.xs(['power_governor', cap])))
        mean_runtime_gov = edf.xs(['power_governor', cap])['runtime (s)'].mean()
        max_runtime_gov = edf.xs(['power_governor', cap])['runtime (s)'].max()
        max_runtime_bal = edf.xs(['power_balancer', cap])['runtime (s)'].max()
        margin = margin_factor * (max_runtime_gov - mean_runtime_gov)
        runtime_pct = (max_runtime_gov - max_runtime_bal) / max_runtime_gov

        result.loc[cap] = [max_runtime_gov, max_runtime_bal, margin, runtime_pct]
    return result


def print_summary(df):
    sys.stdout.write('{}\n'.format(df))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_show_details(parser)

    args = parser.parse_args()
    output_dir = args.output_dir
    show_details = args.show_details

    # TODO: throw if data is missing; mention that user needs to run a
    #       power sweep

    output = geopmpy.io.RawReportCollection("*report", dir_name=output_dir)

    # Test if Epochs were used
    if output.get_epoch_df()[:1]['count'].item() > 0.0:
        df = output.get_epoch_df()
    else:
        df = output.get_app_df()

    result = balancer_comparison(df, detailed=show_details)
    print_summary(result)
