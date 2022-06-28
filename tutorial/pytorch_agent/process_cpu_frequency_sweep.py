#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
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

def process_report_files(input_dir, nodename, app_index):
    reports = list()
    for report_path in chain(
            glob.iglob(os.path.join(input_dir, "*", '*.report')),
            glob.iglob(os.path.join(input_dir, '*.report'))):
        with open(report_path) as f:
            report = yaml.safe_load(f)
        experiment_name = os.path.splitext(os.path.basename(report_path))[0]
        directory_name = os.path.basename(os.path.dirname(report_path))

        name_parts = experiment_name.split('_')
        app_name = name_parts[0]
        for part in name_parts:
            if part.endswith('c'):
                try:
                    core_freq = float(part[:-1])
                except:
                    pass
            if part.endswith('u'):
                try:
                    uncore_freq = float(part[:-1])
                except:
                    pass
        freq_index = name_parts.index('frequency')
        if name_parts[freq_index + 1] == 'map':
            try:
                core_freq = float(name_parts[freq_index + 2])
            except:
                pass
        trial = int(name_parts[-1])

        if "Regions" in report["Hosts"][nodename]:
            for region_dict in report["Hosts"][nodename]["Regions"]:
                region_dict['cpu-frequency'] = core_freq
                region_dict['trial'] = trial
                region_dict['app-config'] = app_name + '-' + directory_name + '-' + hex(region_dict['hash'])
                reports.append(region_dict)
        else:
            # Handle sweeps done with python infrastructure that does not have regions or a region hash
            region_dict = report["Hosts"][nodename]["Application Totals"]
            region_dict['cpu-frequency'] = core_freq
            region_dict['trial'] = trial
            region_dict['app-config'] = app_name + '-' + directory_name + '-' + "0xDEADBEEF"
            reports.append(region_dict)

        if "Unmarked Totals" in report['Hosts'][nodename]:
            region_dict = report["Hosts"][nodename]["Unmarked Totals"]
            region_dict['region'] = "Unmarked"
            region_dict['hash'] = 0x0725e8066
            region_dict['cpu-frequency'] = core_freq
            region_dict['trial'] = trial
            region_dict['app-config'] = app_name + '-' + directory_name + '-' + hex(region_dict['hash'])
            reports.append(region_dict)

    return pd.DataFrame(reports)

def read_trace_files(sweep_dir, nodename, app_index):
    all_dfs = []
    gtf = None
    for trace_file in chain(
            glob.iglob(os.path.join(sweep_dir, "*", f'*.trace-{nodename}')),
            glob.iglob(os.path.join(sweep_dir, f'*.trace-{nodename}'))):
        gtf = trace_file
        trace_df = pd.read_csv(trace_file, sep='|', comment='#', na_values='NAN')
        trace_df['node'] = nodename
        experiment_name = os.path.splitext(os.path.basename(trace_file))[0]
        directory_name = os.path.basename(os.path.dirname(trace_file))
        trace_df['app-index'] = app_index
        name_parts = experiment_name.split('_')
        app_name = name_parts[0]
        for part in name_parts:
            if part.endswith('c'):
                try:
                    core_freq = float(part[:-1])
                except:
                    pass
            if part.endswith('u'):
                try:
                    uncore_freq = float(part[:-1])
                except:
                    pass
        freq_index = name_parts.index('frequency')
        if name_parts[freq_index + 1] == 'map':
            try:
                core_freq = float(name_parts[freq_index + 2])
            except:
                pass
        trial = int(name_parts[-1])

        trace_df['cpu-frequency'] = core_freq
        trace_df['trial'] = trial

        # Handle sweeps done with python infrastructure that does not have a region hash
        if "REGION_HASH" not in trace_df.columns:
            trace_df['REGION_HASH'] = "0xDEADBEEF"
        if 'CPU_POWER' not in trace_df:
            trace_df['CPU_POWER'] = trace_df['CPU_POWER-board-0']
        if 'CPU_FREQUENCY_STATUS' not in trace_df:
            trace_df['CPU_FREQUENCY_STATUS'] = trace_df['CPU_FREQUENCY_STATUS-board-0']
        if 'TIME' not in trace_df:
            trace_df['TIME'] = trace_df['TIME-board-0']
        if 'CPU_CORE_TEMPERATURE' not in trace_df:
            trace_df['CPU_CORE_TEMPERATURE'] = trace_df['CPU_CORE_TEMPERATURE-board-0']

        # Help uniquely identify different configurations of a single app
        config_name = app_name + "-" + directory_name + '-' + trace_df['REGION_HASH']

        trace_df['app-config'] = config_name

        all_dfs.append(trace_df)
    return pd.concat(all_dfs, ignore_index=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Process a set of application frequency sweeps, in prepration '
        'to train a model for a frequency selection agent. Expects that each '
        'output directory contains a series of report files ending with .report '
        'and .trace-{node_name}. The parts of the file names before the file '
        'extensions contain a series of key/value pairs identifying the report '
        'and trace. An example expected report name is: '
        'arithmetic_intensity_frequency_map_<corefreq_in_GHz>e+09_<trial>.report'
        'Explanations of the parts are:\n'
        'corefreq_in_GHz: CPU frequency limit applied on this report.\n'
        'trial: number of the trial of repeated executions.\n'
    )
    parser.add_argument('nodename', help='Which node to analyze.')
    parser.add_argument('output',
                        help='Name of the HDF file to write the output. '
                             'The h5 extension will be added')
    parser.add_argument('frequency_sweep_dirs',
                        nargs='+',
                        help='Directories containing reports and traces from frequency sweeps')
    args = parser.parse_args()

    nodename = args.nodename

    processed_sweeps = list()
    for app_index, full_sweep_dir in enumerate(args.frequency_sweep_dirs):
        reports_df = process_report_files(full_sweep_dir, nodename, app_index)
        #Effectively splitting each report into sub reports based on avx level, region, trial

        config_groups = reports_df.groupby('app-config', group_keys=False, dropna=False)
        relative_runtime = config_groups.apply(lambda x: x['runtime (s)'] / x['runtime (s)'].min())

        # Pandas auto-transposes your data if it has only 1 group. We don't want that.
        reports_df['runtime-vs-maxfreq'] = relative_runtime.T if config_groups.ngroups == 1 else relative_runtime

        max_cpu_freq = reports_df['cpu-frequency'].max()
        # Note that these are all trained against GPU 0. If more GPUs are
        # used at inference time, the model is replicated across the GPUs.
        relative_energy = config_groups.apply(
            lambda x: x['package-energy (J)'] / (x.loc[x['cpu-frequency'] == max_cpu_freq,
                        'package-energy (J)']).mean())
        reports_df['energy-vs-max'] = relative_energy.T if config_groups.ngroups == 1 else relative_energy
        reports_df['performance-loss'] = reports_df['runtime-vs-maxfreq'] - 1

        # Get the min-energy frequency for each config
        min_energy_frequencies = reports_df.pivot_table(
                values='package-energy (J)',
                index=['app-config'],
                columns='cpu-frequency').idxmin(axis=1).rename('Min-Energy CPU Frequency')

        # Generate mulitple columns of data. Each column represents a different
        # weighted importance of performance and energy objectives. The values
        # in each column represent the frequency control that should achieve
        # the target balance of importances.
        tradeoffs_by_frequency = reports_df[['app-config', 'runtime-vs-maxfreq', 'energy-vs-max', 'cpu-frequency']].set_index(['cpu-frequency'])
        perf_frequencies = list()
        for energy_weight in np.linspace(0, 1, 11):
            energy_weight = round(energy_weight, 2)
            objective_column_name = f'energy_weight{int(energy_weight*100)}'
            tradeoffs_by_frequency[objective_column_name] = (
                tradeoffs_by_frequency['runtime-vs-maxfreq'] * (1-energy_weight)
                + tradeoffs_by_frequency['energy-vs-max'] * (energy_weight)
            )
            perf_freq = tradeoffs_by_frequency.groupby(
                'app-config', group_keys=False, dropna=False)[objective_column_name].idxmin()
            perf_frequencies.append(perf_freq)

        # Add the generated columns of report data to each trace file. Melt the
        # generated data such that a new "phi" column in the trace represents
        # which of those columns is being represented in the trace, and a new
        # "phi-freq" column represents which frequency controls are defined by
        # that mapping. In other words, each trace increases in width by 2
        # columns and increases in height by the number of columns generated in
        # the loop before this block.
        df_traces = read_trace_files(full_sweep_dir, nodename, app_index)

        if reports_df['app-config'].count().max() > 0:
            # We did not group the traces based on REGION_HASH, however we are merging them
            # into the grouping provided by the reports, which grouped by REGION_HASH
            df_traces = df_traces.merge(min_energy_frequencies, how='left', on='app-config')
        else:
            df_traces['Min-Energy CPU Frequency'] = min_energy_frequencies

        for perf_freq in perf_frequencies:
            df_traces = df_traces.merge(perf_freq, how='left', on='app-config')
        perf_cols = [c for c in df_traces.columns if c.startswith('energy_weight')]
        for c in perf_cols:
            new_df = df_traces.copy()
            new_df['phi'] = float(c[len('energy_weight'):]) / 100
            new_df['phi-freq'] = new_df[c]
            processed_sweeps.append(new_df)

    # Save the processed sweep data to an hdf file. The reason for doing this as
    # a separate script instead of doing it as part of the model-training script
    # is so that we can easily process all of the sweep traces once to train
    # multiple models with different applications ignored. This lets us spend
    # less time preprocessing for leave-one-out testing.
    pd.concat(processed_sweeps).to_hdf(
        args.output + ".h5", nodename + "_training_data", mode='w')
