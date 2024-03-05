#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import seaborn as sns
import pandas as pd
import os
import argparse
import yaml
import pathlib

from experiment import common_args
from experiment import plotting

def reports_and_traces_to_dataframes(report_paths):
    runs = list()
    core_frequencies = dict()
    epoch_dfs = list()
    core_frequencies_by_time = list()
    for report_path in report_paths:
        with open(report_path, 'r') as report_fd:
            report = yaml.load(report_fd, Loader=yaml.SafeLoader)

            if 'Agent' not in report:
                print('Warning: skipping incomplete report:', report_path)
                continue

            if report['Agent'] == 'frequency_balancer':
                using_pstates = report['Policy']['USE_FREQUENCY_LIMITS']
                using_sst_tf = report['Policy']['USE_SST_TF']
                variant = ('Combined' if (using_pstates and using_sst_tf)
                           else 'P-States' if using_pstates
                           else 'SST-TF')
                agent = f'frequency_balancer ({variant})'
            else:
                agent = report['Agent']
                variant = agent.replace('_', ' ').title()

            time_to_solution = max(data['Application Totals']['runtime (s)'] for host, data in report['Hosts'].items())
            package_energy_to_solution = sum(data['Application Totals']['package-energy (J)'] for host, data in report['Hosts'].items())
            dram_energy_to_solution = sum(data['Application Totals']['dram-energy (J)'] for host, data in report['Hosts'].items())

            app_time_by_core = [time
                                for host, data in report['Hosts'].items()
                                for c,time in data['Application Totals'].items()
                                if c.startswith('TIME@core-')]
            app_network_time_by_core = [time
                                for host, data in report['Hosts'].items()
                                for c,time in data['Application Totals'].items()
                                if c.startswith('TIME_HINT_NETWORK@core-')]
            app_non_network_time_by_core = [time - net_time
                for time, net_time
                in zip(app_time_by_core, app_network_time_by_core)
                if time > 0]

            non_net_time_core_total = sum(app_non_network_time_by_core)
            non_net_time_core_count = len(app_non_network_time_by_core)
            non_net_time_core_max = max(app_non_network_time_by_core)

            # Report imbalance as a percentage where 0% represents perfect
            # balance across all processing units and 100% represents the case
            # where all except one processing unit are waiting on a single
            # processing unit (as in a serial execution region). For more
            # information about this metric, see "Detecting Application Load
            # Imbalance on High End Massively Parallel Systems" by DeRose,
            # Homer, and Johnson (2007).
            imbalance = (
                (non_net_time_core_max - non_net_time_core_total / non_net_time_core_count)
                * non_net_time_core_count
                / (non_net_time_core_max * (non_net_time_core_count - 1))
            )

            app_name = report['Profile'][:report['Profile'].index('_' + report['Agent'])]
            trial = int(report['Profile'].rsplit('_', 1)[-1])

            runs.append({
                'Profile': report['Profile'],
                'Application': app_name,
                'Agent': agent,
                'Variant': variant,
                'trial': trial,
                'Time to Solution': time_to_solution,
                'CPU Package Energy': package_energy_to_solution,
                'DRAM Energy': dram_energy_to_solution,
                'Energy to Solution': package_energy_to_solution + dram_energy_to_solution,
                'Imbalance': imbalance,
            })

            for host, data in report['Hosts'].items():
                trace_path = report_path.parent.joinpath(report_path.stem + f'.trace-{host}')
                for field,value in data['Application Totals'].items():
                    if field.startswith('CPU_FREQUENCY_STATUS@core-') and value != 0:
                        core = int(field.split('@core-', 1)[-1])
                        core_frequencies[(report['Profile'], core)] = {
                            'Profile': report['Profile'],
                            'Application': app_name,
                            'Agent': agent,
                            'Variant': variant,
                            'trial': trial,
                            'Frequency': value,
                        }

                # Determine the processor's sticker frequency from the measured
                # absolute frequency and the measured frequency as a percentage
                # of sticker. Round down to 100 MHz steps.
                sticker_frequency = (
                    data['Application Totals']['frequency (Hz)']
                    / data['Application Totals']['frequency (%)']
                    * 100) // 1e8 * 1e8

                trace_df = pd.read_csv(trace_path, sep='|', comment='#', na_values='NAN')
                trace_df['timediff'] = trace_df['TIME'].diff()
                core_count = len([c for c in trace_df.columns if c.startswith('MSR::APERF:ACNT-core')])
                for core in range(core_count):
                    # 0x00000004 indicates that GEOPM's network hint is active
                    is_networking = (trace_df[f'REGION_HINT-core-{core}'] == '0x00000004')
                    trace_df[f'network-time-core-{core}'] = is_networking * trace_df['timediff']
                    diff_acnt = trace_df[f'MSR::APERF:ACNT-core-{core}'].diff()
                    diff_mcnt = trace_df[f'MSR::MPERF:MCNT-core-{core}'].diff()
                    if (report['Profile'], core) in core_frequencies:
                        if (~is_networking).sum() > 0:
                            core_frequencies[(report['Profile'], core)]['Non-Networking Frequency'] = diff_acnt.loc[~is_networking].sum() / diff_mcnt.loc[~is_networking].sum() * sticker_frequency
                        if is_networking.sum() > 0:
                            core_frequencies[(report['Profile'], core)]['Networking Frequency'] = diff_acnt.loc[is_networking].sum() / diff_mcnt.loc[is_networking].sum() * sticker_frequency

                acnt_mcnt_by_epoch = trace_df.pivot_table(
                    [c for c in trace_df.columns if c.startswith('MSR::APERF:ACNT-core') or c.startswith('MSR::MPERF:MCNT-core')],
                    'EPOCH_COUNT',
                    aggfunc='last').diff().dropna()
                time_by_epoch = trace_df.pivot_table(
                    'TIME',
                    'EPOCH_COUNT',
                    aggfunc='last').diff().dropna()
                network_time_by_epoch = trace_df.pivot_table(
                    [c for c in trace_df.columns if c.startswith('network-time-core') or c.startswith('network-time-core')],
                    'EPOCH_COUNT',
                    aggfunc='sum').loc[1:] # ignore epoch 0 (before the main loop)

                desired_time_by_epoch = None
                if 'DESIRED_NON_NETWORK_TIME-package-0' in trace_df:
                    desired_time_by_epoch = pd.wide_to_long(
                        trace_df.pivot_table(
                            [c for c in trace_df.columns if c.startswith('DESIRED_NON_NETWORK_TIME-package')],
                            'EPOCH_COUNT',
                            aggfunc='last').loc[1:].reset_index(),
                        'DESIRED_NON_NETWORK_TIME', 'EPOCH_COUNT', 'package', sep='-package-')

                desired_cutoff_by_epoch = None
                if 'DESIRED_CUTOFF_FREQUENCY-package-0' in trace_df:
                    desired_cutoff_by_epoch = pd.wide_to_long(
                        trace_df.pivot_table(
                            [c for c in trace_df.columns if c.startswith('DESIRED_CUTOFF_FREQUENCY-package')],
                            'EPOCH_COUNT',
                            aggfunc='last').loc[1:].reset_index(),
                        'DESIRED_CUTOFF_FREQUENCY', 'EPOCH_COUNT', 'package', sep='-package-')

                stubs={c[:c.index('-core-')] for c in trace_df.columns if '-core-' in c}
                percore_trace_df = pd.wide_to_long(trace_df.drop(stubs, axis=1, errors='ignore'), stubnames=stubs, i='TIME', j='core', sep='-core-')

                epoch_df = pd.DataFrame(index=acnt_mcnt_by_epoch.index)
                epoch_df.columns.name = 'Core'
                for core in range(core_count):
                    if trace_df[f'REGION_HASH-core-{core}'].notna().sum() > 0:
                        epoch_df[f'Frequency-core-{core}'] = (
                            acnt_mcnt_by_epoch[f'MSR::APERF:ACNT-core-{core}']
                            / acnt_mcnt_by_epoch[f'MSR::MPERF:MCNT-core-{core}']
                            * sticker_frequency)
                        epoch_df[f'Network Time-core-{core}'] = network_time_by_epoch[f'network-time-core-{core}']
                epoch_df = pd.wide_to_long(epoch_df.reset_index(), stubnames=['Network Time', 'Frequency'], i='EPOCH_COUNT', j='Core', sep='-core-').reset_index().rename(
                    {'EPOCH_COUNT': 'Epoch', 'Network Tim': 'Network Time'}, axis=1) # Pandas is truncating the name for some reason
                # epoch_df.stack().rename('Frequency').reset_index().rename({'EPOCH_COUNT': 'Epoch'}, axis=1)
                epoch_df['Time'] = epoch_df['Epoch'].map(time_by_epoch['TIME'])
                epoch_df['Non-Network Time'] = epoch_df['Time'] - epoch_df['Network Time']
                epoch_df['Package'] = epoch_df['Core'] // (core_count // 2)
                epoch_df['Package Core'] = epoch_df['Core'] % (core_count // 2)
                if desired_time_by_epoch is not None:
                    epoch_df['Desired Time'] = epoch_df[['Epoch', 'Package']].apply(tuple, axis=1).map(desired_time_by_epoch['DESIRED_NON_NETWORK_TIME'])
                else:
                    epoch_df['Desired Cutoff'] = float('inf')
                if desired_cutoff_by_epoch is not None:
                    epoch_df['Desired Cutoff'] = epoch_df[['Epoch', 'Package']].apply(tuple, axis=1).map(desired_cutoff_by_epoch['DESIRED_CUTOFF_FREQUENCY'])
                else:
                    epoch_df['Desired Time'] = float('inf')
                epoch_df['Host'] = host
                epoch_df['Application'] = app_name
                epoch_df['Variant'] = variant
                epoch_df['trial'] = trial
                epoch_critical_path_core = epoch_df.pivot('Epoch', 'Core', 'Non-Network Time').idxmax(axis=1)
                epoch_df['Critical Path Frequency'] = epoch_df.loc[epoch_df['Core'] == epoch_df['Epoch'].map(epoch_critical_path_core), 'Frequency']

                epoch_dfs.append(epoch_df)

                frequency_by_time = pd.DataFrame(index=trace_df['TIME'])
                frequency_by_time.columns.name = 'Core'
                for core in range(core_count):
                    if trace_df[f'REGION_HASH-core-{core}'].notna().sum() > 0:
                        # If we're in here, then at least one sample for this core had a region hash.
                        # Otherwise, assume it is a non-application core.
                        frequency_by_time[core] = (
                            trace_df.set_index('TIME')[f'MSR::APERF:ACNT-core-{core}'].diff()
                            / trace_df.set_index('TIME')[f'MSR::MPERF:MCNT-core-{core}'].diff()
                            * sticker_frequency)
                frequency_by_time = frequency_by_time.stack().rename('Frequency').reset_index().rename({'TIME': 'Time'}, axis=1)
                frequency_by_time['Package'] = frequency_by_time['Core'] // (core_count // 2)
                frequency_by_time['Package Core'] = frequency_by_time['Core'] % (core_count // 2)
                frequency_by_time['Host'] = host
                frequency_by_time['Application'] = app_name
                frequency_by_time['Variant'] = variant
                frequency_by_time['trial'] = trial
                core_frequencies_by_time.append(frequency_by_time)

    df = pd.DataFrame(runs)
    reference_time = df.loc[df['Agent'] == 'monitor'].groupby('Application')['Time to Solution'].mean()
    df['Time Savings'] = 1 - df['Time to Solution'] / df['Application'].map(reference_time)

    reference_energy = df.loc[df['Agent'] == 'monitor'].groupby('Application')['Energy to Solution'].mean()
    df['Energy Savings'] = 1 - df['Energy to Solution'] / df['Application'].map(reference_energy)

    frequency_df = pd.DataFrame(core_frequencies.values())
    epoch_df = pd.concat(epoch_dfs)

    core_frequencies_by_time_df = pd.concat(core_frequencies_by_time)

    return df, frequency_df, epoch_df, core_frequencies_by_time_df

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_analysis_dir(parser)

    parser.add_argument('report_paths', type=pathlib.Path, nargs='+', help='Paths to report files')
    parser.add_argument('--all-core-turbo-hz', type=float, help='All-core turbo limit, in hertz')
    parser.add_argument('--plot-suffix', default='png', help='File suffix to use for plots')
    args, _ = parser.parse_known_args()

    os.makedirs(args.analysis_dir, exist_ok=True)
    os.makedirs(args.output_dir, exist_ok=True)

    try:
        # Try to load the preprocessed data from a cached file
        df = pd.read_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'df')
        frequency_df = pd.read_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'frequency_df')
        epoch_df = pd.read_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'epoch_df')
        core_frequencies_by_time_df = pd.read_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'core_frequencies_by_time_df')
    except OSError:
        # Write the preprocessed data to a cached file
        df, frequency_df, epoch_df, core_frequencies_by_time_df = reports_and_traces_to_dataframes(args.report_paths)
        df.to_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'df', 'w')
        frequency_df.to_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'frequency_df', 'a')
        epoch_df.to_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'epoch_df', 'a')
        core_frequencies_by_time_df.to_hdf(os.path.join(args.analysis_dir, 'cache.hdf'), 'core_frequencies_by_time_df', 'a')

    df['Application'] = df['Application'].str.rsplit('_', 1).str[-1] #TODO debug shortname workaround
    epoch_df['Application'] = epoch_df['Application'].str.rsplit('_', 1).str[-1] #TODO debug shortname workaround
    frequency_df['Application'] = frequency_df['Application'].str.rsplit('_', 1).str[-1] #TODO debug shortname workaround

    epoch_df = epoch_df.loc[epoch_df['Epoch'] > 30]
    epoch_df = epoch_df.loc[epoch_df['Epoch'] < 50]

    sns.set_theme(context='paper', style='whitegrid', palette='colorblind')

    variant_order = ['Monitor', 'P-States', 'SST-TF', 'Combined', 'Power Governor']
    variant_order = [v for v in variant_order if v in df['Variant'].unique()]
    variant_palette = dict(zip(variant_order, sns.color_palette(n_colors=len(variant_order))))

    fig, ax = plt.subplots(figsize=(4, 2))
    sns.barplot(
        ax=ax,
        data=df.loc[df['Agent'] != 'monitor'],
        hue='Variant',
        hue_order=[v for v in variant_order if v != 'Monitor'],
        palette=variant_palette,
        x='Application',
        y='Time Savings',
    )
    ax.yaxis.set_major_formatter(mtick.PercentFormatter(1, 0))
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    fig.suptitle('Time Savings by Application')
    fig.savefig(os.path.join(args.output_dir, f'time-to-solution.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')

    fig, ax = plt.subplots(figsize=(4, 2))
    sns.barplot(
        ax=ax,
        data=df.loc[df['Agent'] != 'monitor'],
        hue='Variant',
        hue_order=[v for v in variant_order if v != 'Monitor'],
        palette=variant_palette,
        x='Application',
        y='Energy Savings',
    )
    ax.yaxis.set_major_formatter(mtick.PercentFormatter(1, 0))
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    fig.suptitle('Energy Savings by Application')
    fig.savefig(os.path.join(args.output_dir, f'energy-to-solution.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')


    fig, ax = plt.subplots(figsize=(4, 2))
    sns.barplot(
        ax=ax,
        data=df,
        hue='Variant',
        hue_order=variant_order,
        palette=variant_palette,
        x='Application',
        y='Imbalance',
    )
    ax.yaxis.set_major_formatter(mtick.PercentFormatter(1, 0))
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    fig.suptitle('Imbalance by Application')
    fig.savefig(os.path.join(args.output_dir, f'imbalance.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')


    fig, ax = plt.subplots(figsize=(4, 2))
    sns.violinplot(
        data=epoch_df,
        ax=ax,
        inner=None,
        dodge=True,
        linewidth=0.5,
        cut=0,
        scale='count',
        hue='Variant',
        hue_order=variant_order,
        palette=variant_palette,
        x='Application',
        y='Frequency',
    )
    if args.all_core_turbo_hz is not None:
        ax.axhline(y=args.all_core_turbo_hz, ls=":", c='k', linewidth=0.5, label='All-Core Turbo')
    ax.yaxis.set_major_formatter(mtick.EngFormatter('Hz', 0))
    ax.set_ylim(bottom=0)
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    fig.suptitle('CPU Core Frequencies by Application')
    fig.savefig(os.path.join(args.output_dir, f'frequencies.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')


    fig, ax = plt.subplots(figsize=(4, 2))
    sns.violinplot(
        data=epoch_df,
        ax=ax,
        inner=None,
        dodge=True,
        linewidth=0.5,
        cut=0,
        scale='count',
        hue='Variant',
        hue_order=variant_order,
        palette=variant_palette,
        x='Application',
        y='Critical Path Frequency',
    )
    if args.all_core_turbo_hz is not None:
        ax.axhline(y=args.all_core_turbo_hz, ls=":", c='k', linewidth=0.5, label='All-Core Turbo')
    ax.yaxis.set_major_formatter(mtick.EngFormatter('Hz', 0))
    ax.set_ylim(bottom=0)
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    ax.set_ylabel('Frequency')

    fig.suptitle('Critical Path Frequencies by Application')
    fig.savefig(os.path.join(args.output_dir, f'critical-path-frequencies.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')


#    fig, ax = plt.subplots(figsize=(4, 2))
#    sns.violinplot(
#        data=frequency_df,
#        ax=ax,
#        inner=None,
#        dodge=True,
#        linewidth=0.5,
#        cut=0,
#        scale='count',
#        hue='Variant',
#        hue_order=variant_order,
#        palette=variant_palette,
#        x='Application',
#        y='Non-Networking Frequency',
#    )
#    if args.all_core_turbo_hz is not None:
#        ax.axhline(y=args.all_core_turbo_hz, ls=":", c='k', linewidth=0.5, label='All-Core Turbo')
#    ax.set_ylabel('Frequency')
#    ax.yaxis.set_major_formatter(mtick.EngFormatter('Hz', 0))
#    ax.set_ylim(bottom=0)
#    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
#
#    fig.suptitle('Non-Networking Core Frequencies by Application')
#    fig.savefig(os.path.join(args.output_dir, f'non-networking-frequencies.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')
#
#
#    fig, ax = plt.subplots(figsize=(4, 2))
#    sns.violinplot(
#        data=frequency_df,
#        ax=ax,
#        inner=None,
#        dodge=True,
#        linewidth=0.5,
#        cut=0,
#        scale='count',
#        hue='Variant',
#        hue_order=variant_order,
#        palette=variant_palette,
#        x='Application',
#        y='Networking Frequency',
#    )
#    if args.all_core_turbo_hz is not None:
#        ax.axhline(y=args.all_core_turbo_hz, ls=":", c='k', linewidth=0.5, label='All-Core Turbo')
#    ax.set_ylabel('Frequency')
#    ax.yaxis.set_major_formatter(mtick.EngFormatter('Hz', 0))
#    ax.set_ylim(bottom=0)
#    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
#
#    fig.suptitle('Networking Core Frequencies by Application')
#    fig.savefig(os.path.join(args.output_dir, f'networking-frequencies.{args.plot_suffix}'), pad_inches=0, bbox_inches='tight')
#
    for application,data in epoch_df.groupby('Application'):
        plot_data = data.loc[(data['trial'] == 1) & (data['Host'] == 'mcfly5')]
        g = sns.FacetGrid(
            height=2.5,
            aspect=3,
            #facet_kws={'sharey': 'row'},
            data=plot_data,
            row='Variant',
            col='Package',
        )
        if 'Desired Cutoff' in plot_data.columns:
            g.map_dataframe(sns.lineplot, 'Epoch', 'Desired Cutoff', linewidth=0.8, color='b', zorder=3)
        g.map_dataframe(
            sns.lineplot,
            ci=None,
            hue='Package Core',
            #linewidth=0,
            #s=4,#linewidth=1,
            #units='Core',
            estimator=None,
            x='Epoch',
            y='Frequency',
        )
        for ax in g.axes.flatten():
            ax.yaxis.set_major_formatter(mtick.EngFormatter('Hz', 2))
            ax.set_xlim(plot_data['Epoch'].min(), plot_data['Epoch'].max())
        g.fig.suptitle(f'CPU Core Frequencies in {application}')
        g.savefig(os.path.join(args.output_dir, f'core-frequencies-{application.replace("_", "-")}.{args.plot_suffix}'))

        g = sns.FacetGrid(
            height=2.5,
            aspect=3,
            #facet_kws={'sharey': 'row'},
            data=plot_data,
            row='Variant',
            col='Package',
        )
        if 'Desired Time' in plot_data.columns:
            g.map_dataframe(sns.lineplot, 'Epoch', 'Desired Time', linewidth=0.8, color='b', zorder=3)
        g.map_dataframe(
            sns.lineplot,
            ci=None,
            hue='Package Core',
            #linewidth=0,
            #s=4,#linewidth=1,
            #units='Core',
            estimator=None,
            x='Epoch',
            y='Non-Network Time',
        )
        for ax in g.axes.flatten():
            ax.set_xlim(plot_data['Epoch'].min(), plot_data['Epoch'].max())
        g.fig.suptitle(f'CPU Core Non-Network Time in {application}')
        g.savefig(os.path.join(args.output_dir, f'core-non-network-time-{application.replace("_", "-")}.{args.plot_suffix}'))

        g = sns.relplot(
            height=2.5,
            aspect=3,
            #facet_kws={'sharey': 'row'},
            data=plot_data,
            kind='line',
            ci=None,
            hue='Package Core',
            #linewidth=0,
            #s=4,#linewidth=1,
            #units='Core',
            estimator=None,
            row='Variant',
            col='Package',
            x='Epoch',
            y='Network Time',
        )
        for ax in g.axes.flatten():
            ax.set_xlim(plot_data['Epoch'].min(), plot_data['Epoch'].max())
        g.fig.suptitle(f'CPU Core Network Time in {application}')
        g.savefig(os.path.join(args.output_dir, f'core-network-time-{application.replace("_", "-")}.{args.plot_suffix}'))

        g = sns.relplot(
            height=2.5,
            aspect=3,
            #facet_kws={'sharey': 'row'},
            data=plot_data,
            kind='line',
            ci=None,
            hue='Package Core',
            #linewidth=0,
            #s=4,#linewidth=1,
            #units='Core',
            estimator=None,
            row='Variant',
            col='Package',
            x='Epoch',
            y='Time',
        )
        for ax in g.axes.flatten():
            ax.set_xlim(plot_data['Epoch'].min(), plot_data['Epoch'].max())
        g.fig.suptitle(f'CPU Core Time in {application}')
        g.savefig(os.path.join(args.output_dir, f'core-time-{application.replace("_", "-")}.{args.plot_suffix}'))
