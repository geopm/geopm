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
Example power sweep experiment using geopmbench.
'''

import sys
import pandas
import argparse

import geopmpy.io

from experiment import common_args


def balancer_comparison(report_epoch_data, detailed=False):
    margin_factor = 0.25
    power_caps = report_epoch_data['POWER_PACKAGE_LIMIT_TOTAL'].dropna().unique() # dropna handles monitor runs

    result = pandas.DataFrame(columns=['max_runtime_gov', 'max_runtime_bal', 'margin', 'runtime_pct'])
    for cap in power_caps:
        edf = report_epoch_data
        edf = edf.set_index(['Agent', 'POWER_PACKAGE_LIMIT_TOTAL', 'Profile', 'host'])
        if detailed:
            sys.stdout.write('{}\n'.format(edf.xs(['power_governor', cap])))
        mean_runtime_gov = edf.xs(['power_governor', cap])['runtime (sec)'].mean()
        max_runtime_gov = edf.xs(['power_governor', cap])['runtime (sec)'].max()
        max_runtime_bal = edf.xs(['power_balancer', cap])['runtime (sec)'].max()
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
