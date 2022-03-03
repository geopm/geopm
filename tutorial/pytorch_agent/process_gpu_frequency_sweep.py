#!/usr/bin/env python3

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
import pandas as pd
import numpy as np
import os
import glob
import yaml
import argparse
import code

def process_report_files(input_dir, nodename, app_index):
    reports = list()
    for report_path in glob.iglob(os.path.join(input_dir, '*.report')):
        with open(report_path) as f:
            report = yaml.safe_load(f)
        experiment_name = os.path.splitext(os.path.basename(report_path))[0]
        name_parts = [part.split('_') for part in experiment_name.split('.') if '_' in part]
        app_totals = report['Hosts'][nodename]['Application Totals']
        app_totals['app-index'] = app_index
        app_totals['gpu-frequency'] = np.nan
        app_totals['trial'] = np.nan

        # Help uniquely identify different configurations of a single app
        config_name = str(app_index)

        for part, value in name_parts:
            if part == 'gpufreq':
                app_totals['gpu-frequency'] = int(value) * 1e6
            if part == 'memratio':
                config_name += '-' + value
            if part == 'precision':
                config_name += '-' + value
            if part == 'trial':
                app_totals['trial'] = int(value)
        app_totals['app-config'] = config_name

        reports.append(app_totals)
    return pd.DataFrame(reports)


def read_trace_files(sweep_dir, nodename, app_index):
    all_dfs = []
    for trace_file in glob.iglob(os.path.join(sweep_dir, f'*.trace-{nodename}')):
        trace_df = pd.read_csv(trace_file, sep='|', comment='#', na_values='NAN')
        trace_df['node'] = nodename
        experiment_name = os.path.splitext(os.path.basename(trace_file))[0]
        trace_df['app-index'] = app_index
        name_parts = [part.split('_') for part in experiment_name.split('.') if '_' in part]
        trace_df['gpu-frequency'] = np.nan
        trace_df['trial'] = np.nan

        # Help uniquely identify different configurations of a single app
        config_name = str(app_index)

        for part, value in name_parts:
            if part == 'gpufreq':
                trace_df['gpu-frequency'] = int(value) * 1e6
            if part == 'memratio':
                config_name += '-' + value
            if part == 'precision':
                config_name += '-' + value
            if part == 'trial':
                trace_df['trial'] = int(value)
        trace_df['app-config'] = config_name

        # Apply a heuristic to ignore setup/teardown parts of workloads. Treat
        # the region of interest as the section of the trace between the first
        # and last observed instance of moderate GPU activity.
        is_gpu_util = trace_df['GPU_COMPUTE_ACTIVITY-board_accelerator-0'] > 0.05
        if is_gpu_util.sum() == 0:
            is_gpu_util = trace_df['GPU_UTILIZATION-board_accelerator-0'] > 0.05
        first_util = trace_df.loc[is_gpu_util].index[0]
        last_util = trace_df.loc[is_gpu_util].index[-1]
        trace_df['is-roi'] = False
        trace_df.loc[first_util:last_util, 'is-roi'] = True

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
        'dgemm.gpufreq_<freq_in_GHz>.memratio_<mem_ratio>.precision_<precision>.trial_<trial>.report'
        'Explanations of the parts are:\n'
        'freq_in_GHz: GPU core frequency limit applied on this report.\n'
        'memratio: mem ratio for runs of the mixbench benchmark, or NaN.\n'
        'precision: precision for runs of the mixbench benchmark, or NaN.\n'
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

        config_groups = reports_df.groupby('app-config', group_keys=False, dropna=False)
        relative_runtime = config_groups.apply(lambda x: x['runtime (s)'] / x['runtime (s)'].min())

        # Pandas auto-transposes your data if it has only 1 group. We don't want that.
        reports_df['runtime-vs-maxfreq'] = relative_runtime.T if config_groups.ngroups == 1 else relative_runtime

        max_gpu_freq = reports_df['gpu-frequency'].max()
        # Note that these are all trained against GPU 0. If more GPUs are
        # used at inference time, the model is replicated across the GPUs.

        code.interact(local=locals())
        relative_energy = config_groups.apply(
            lambda x: x['GPU_ENERGY-board_accelerator-0'] / (x.loc[x['gpu-frequency'] == max_gpu_freq,
                        'GPU_ENERGY-board_accelerator-0']).mean())
        reports_df['energy-vs-max'] = relative_energy.T if config_groups.ngroups == 1 else relative_energy
        reports_df['performance-loss'] = reports_df['runtime-vs-maxfreq'] - 1

        # Get the min-energy frequency for each config
        min_energy_frequencies = reports_df.pivot_table(
                values='GPU_ENERGY-board_accelerator-0',
                index=['app-config'],
                columns='gpu-frequency').idxmin(axis=1).rename('Min-Energy GPU Frequency')

        # Generate mulitple columns of data. Each column represents a different
        # weighted importance of performance and energy objectives. The values
        # in each column represent the frequency control that should achieve
        # the target balance of importances.
        tradeoffs_by_frequency = reports_df[['app-config', 'runtime-vs-maxfreq', 'energy-vs-max', 'gpu-frequency']].set_index(['gpu-frequency'])
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
            df_traces = df_traces.merge(min_energy_frequencies, how='left', on='app-config')
        else:
            df_traces['Min-Energy GPU Frequency'] = min_energy_frequencies
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
