#!/usr/bin/env python3
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import pandas as pd
import numpy as np
import os
import glob
import yaml
import argparse
import code
from itertools import chain

def process_report_files(input_dir):
    reports = []
    print(f"Processing {input_dir}")
    for report_path in chain(
            glob.iglob(os.path.join(input_dir, "*", '*.report')),
            glob.iglob(os.path.join(input_dir, '*.report'))):
        with open(report_path) as f:
            report = yaml.safe_load(f)

        #Name of application being profiled
        app_name = report["Profile"][:report["Profile"].find('_frequency_map')]
        experiment_name = os.path.splitext(os.path.basename(report_path))[0]

        freqs = {"gpu":None, "cpu":None, "uncore":None}
        lookup_str = {"gpu":"FREQ_GPU_DEFAULT", "cpu":"FREQ_CPU_DEFAULT", "uncore":"FREQ_CPU_UNCORE"}

        for device in freqs:
            if lookup_str[device] in report["Policy"] and report["Policy"][lookup_str[device]] != "NAN":
                freqs[device] =  float(report["Policy"][lookup_str[device]])

        for nodename in report["Hosts"]:
            conf = {}
            conf['node'] = nodename
            for device in freqs:
                if freqs[device] is not None:
                    conf[f"{device}-frequency"] = freqs[device]

            if "Regions" in report["Hosts"][nodename]:
                for region_dict in report["Hosts"][nodename]["Regions"]:
                    region_dict['app-config'] = app_name + '-' + hex(region_dict['hash'])
                    region_dict.update(conf)
                    reports.append(region_dict)
            else:
                # Handle sweeps done with python infrastructure that does not have regions or a region hash
                region_dict = report["Hosts"][nodename]["Application Totals"]
                region_dict['app-config'] = app_name + '-' + "0xDEADBEEF"

                region_dict.update(conf)
                reports.append(region_dict)

            if "Unmarked Totals" in report['Hosts'][nodename]:
                region_dict = report["Hosts"][nodename]["Unmarked Totals"]
                region_dict['region'] = "Unmarked"
                region_dict['hash'] = f"{app_name}_unmarked"
                region_dict['app-config'] = app_name + '-' + region_dict['hash']
                region_dict.update(conf)
                reports.append(region_dict)

    return pd.DataFrame(reports)

# Process trace file to be ingested into HDF
def process_trace_files(sweep_dir):
    all_dfs = []
    for trace_file in chain(
            glob.iglob(os.path.join(sweep_dir, "*", f'*.trace-*')),
            glob.iglob(os.path.join(sweep_dir, f'*.trace-*'))):

        trace_df = pd.read_csv(trace_file, sep='|', comment='#')

        #Capture header information from trace file without ingesting whole file
        trace_header = {}
        for line in open(trace_file, "r"):
            if line[0] != '#':
                break

            #Clean up trace header if there are multiple ':'
            key = line[2:line.strip().find(':')]
            value = line[2+line.strip().find(':'):]
            trace_header[key.strip()] = value.strip()

        nodename = trace_header["node_name"]
        app_name = trace_header["profile_name"][:trace_header["profile_name"].find('_frequency_map')]

        #Filter out nan and "NAN" regions
        trace_df = trace_df[trace_df['REGION_HASH'].notna()]
        trace_df = trace_df[trace_df['REGION_HASH'] != "NAN"]

        trace_df['node'] = nodename

        # Help uniquely identify different configurations of a single app, used to train on
        # instead of REGION_HASH
        trace_df['app-config'] = app_name + '-' + trace_df['REGION_HASH']

        all_dfs.append(trace_df)
    return pd.concat(all_dfs, ignore_index=True)


def main(output_prefix, frequency_sweep_dirs):
    reports_dfs = []

    #Determine which columns to keep for stats file, which is used to generate frequency
    #recommendations
    keep_columns = [
                'node',
                'hash',
                'region',
                'app-config',
                'runtime (s)',
                'cpu-frequency',
                'package-energy (J)',
            ]
    want_columns = [
            'gpu-frequency',
            'gpu-energy (J)',
            'uncore-frequency',
            ]
    first = True
    #TODO: Catch situation where some report is empty
    #      Desired behavior: Skip if others are valid, output warning message w/ report path name
    #                        If all reports are invalid, output error message and quit

    for full_sweep_dir in frequency_sweep_dirs:
        reports_df = process_report_files(full_sweep_dir)
        if first:
            for want_column in want_columns:
                if want_column in reports_df.columns:
                    keep_columns.append(want_column)
        reports_dfs.append(reports_df[keep_columns])
        first = False

    pd \
    .concat(reports_dfs, ignore_index=True) \
    .to_hdf(f"{output_prefix}_stats.h5", "stats", mode='w')

    #Creating trace hdf for training neural net, annotated with region hashes or
    #generated region names when hashes are not available
    trace_dfs = []
    for full_sweep_dir in frequency_sweep_dirs:
        trace_dfs.append(process_trace_files(full_sweep_dir))

    pd \
    .concat(trace_dfs, ignore_index=True) \
    .to_hdf(f"{output_prefix}_traces.h5", "traces", mode='w')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Generate HDFs from frequency sweep reports/traces, to use
                     with ffnet agent integration infrastructure.'
    )
    parser.add_argument('output',
                        help='Prefix name of the output HDF files.')
    parser.add_argument('frequency_sweep_dirs',
                        nargs='+',
                        help='Directories containing reports and traces from frequency sweeps')
    args = parser.parse_args()

    main(args.output, args.frequency_sweep_dirs)
