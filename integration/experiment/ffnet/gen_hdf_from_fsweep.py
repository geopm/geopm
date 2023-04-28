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

def process_report_files(input_dir, app_index):
    reports = []
    for report_path in chain(
            glob.iglob(os.path.join(input_dir, "*", '*.report')),
            glob.iglob(os.path.join(input_dir, '*.report'))):
        with open(report_path) as f:
            report = yaml.safe_load(f)
        experiment_name = os.path.splitext(os.path.basename(report_path))[0]
        directory_name = os.path.basename(os.path.dirname(report_path))

        gpu_freq = None
        cpu_freq = None
        uncore_freq = None

        if 'GPU_FREQUENCY' in report["Policy"]:
            gpu_freq = float(report["Policy"]["GPU_FREQUENCY"])
        if "UNCORE_MIN_FREQUENCY" in report["Policy"] and "UNCORE_MAX_FREQUENCY" in report["Policy"]:
            if float(report["Policy"]["UNCORE_MIN_FREQUENCY"]) == float(report["Policy"]["UNCORE_MAX_FREQUENCY"]):
                uncore_freq = float(report["Policy"]["UNCORE_MIN_FREQUENCY"])
        if "CORE_FREQUENCY" in report["Policy"]:
            cpu_freq = report["Policy"]["CORE_FREQUENCY"]

        name_parts = experiment_name.split('_')
        app_name = name_parts[0]
        #Differentiate between parres dgemm and nstream
        if app_name == "parres":
            app_name = name_parts[1]

        for nodename in report["Hosts"]:
            conf = {}
            if cpu_freq is not None:
                conf['cpu-frequency'] = cpu_freq
            if gpu_freq is not None:
                conf['gpu-frequency'] = gpu_freq
            if uncore_freq is not None:
                conf['uncore-frequency'] = uncore_freq
            conf['node'] = nodename

            if "Regions" in report["Hosts"][nodename]:
                for region_dict in report["Hosts"][nodename]["Regions"]:
                    region_dict['app-config'] = app_name + '-' + directory_name + '-' + hex(region_dict['hash'])
                    region_dict.update(conf)
                    reports.append(region_dict)
            else:
                # Handle sweeps done with python infrastructure that does not have regions or a region hash
                region_dict = report["Hosts"][nodename]["Application Totals"]
                region_dict['app-config'] = app_name + '-' + directory_name + '-' + "0xDEADBEEF"

                region_dict.update(conf)
                reports.append(region_dict)

            if "Unmarked Totals" in report['Hosts'][nodename]:
                region_dict = report["Hosts"][nodename]["Unmarked Totals"]
                region_dict['region'] = "Unmarked"
                region_dict['hash'] = f"{app_name}_unmarked"
                region_dict['app-config'] = app_name + '-' + directory_name + '-' + region_dict['hash']
                region_dict.update(conf)
                reports.append(region_dict)

    return pd.DataFrame(reports)

# Process trace file to be ingested into HDF
def process_trace_files(sweep_dir, app_index):
    all_dfs = []
    for trace_file in chain(
            glob.iglob(os.path.join(sweep_dir, "*", f'*.trace-*')),
            glob.iglob(os.path.join(sweep_dir, f'*.trace-*'))):

        #TODO: Get node name from comments at top of trace file
        nodename = trace_file[trace_file.rfind('.trace-')+7:]

        trace_df = pd.read_csv(trace_file, sep='|', comment='#')

        #Filter out nan and "NAN" regions
        trace_df = trace_df[trace_df['REGION_HASH'].notna()]
        trace_df = trace_df(trace_df['REGION_HASH'] != "NAN")

        trace_df['node'] = nodename
        #TODO: There's got to be a better way to get the application name
        experiment_name = os.path.splitext(os.path.basename(trace_file))[0]
        directory_name = os.path.basename(os.path.dirname(trace_file))
        trace_df['app-index'] = app_index
        name_parts = experiment_name.split('_')
        app_name = name_parts[0]

        # Handle sweeps done with python infrastructure that does not have a region hash
        if "REGION_HASH" not in trace_df.columns:
            #trace_df['REGION_HASH'] = "0xDEADBEEF"
            continue

        trace_df.loc[trace_df['REGION_HASH'] == '0x725e8066', 'REGION_HASH'] = f"{app_name}_unmarked"

        # Handle old signal names present in traces from geopm1
        key_change = [('POWER_PACKAGE', 'CPU_POWER'),
                      ('ENERGY_PACKAGE', 'CPU_ENERGY'),
                      ('INSTRUCTIONS_RETIRED', 'CPU_INSTRUCTIONS_RETIRED'),
                      ('CYCLES_THREAD', 'CPU_CYCLES_THREAD'),
                      ('QM_CTR_SCALED_RATE', 'MSR::QM_CTR_SCALED_RATE'),
                      ('ENERGY_DRAM', 'DRAM_ENERGY'),
                      ('POWER_DRAM', 'DRAM_POWER'),
                      ]

        for column in trace_df.columns:
            for old_key, new_key in key_change:
                if column.startswith(old_key):
                    ncolumn = new_key + column[len(old_key):]
                    trace_df[ncolumn] = trace_df[column]

        # TODO: Find out if there's a cleaner way
        # Help uniquely identify different configurations of a single app
        config_name = app_name + "-" + directory_name + '-' + trace_df['REGION_HASH']

        trace_df['app-config'] = config_name

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
    #      Desired behavior: Skip if others are valid put output warning message
    #                        If all reports are invalid, output error message and quit

    for app_index, full_sweep_dir in enumerate(frequency_sweep_dirs):
        reports_df = process_report_files(full_sweep_dir, app_index)
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
    for app_index, full_sweep_dir in enumerate(frequency_sweep_dirs):
        trace_dfs.append(process_trace_files(full_sweep_dir, app_index))

    pd \
    .concat(trace_dfs, ignore_index=True) \
    .to_hdf(f"{output_prefix}_traces.h5", "traces", mode='w')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='TODO'
    )
    parser.add_argument('output',
                        help='Prefix name of the output HDF files.')
    parser.add_argument('frequency_sweep_dirs',
                        nargs='+',
                        help='Directories containing reports and traces from frequency sweeps')
    args = parser.parse_args()

    main(args.output, args.frequency_sweep_dirs)
