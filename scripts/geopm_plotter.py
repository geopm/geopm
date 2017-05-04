#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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
"""
GEOPM Plotter - Used to produce plots and other analysis files from report and/or trace files.
"""

import sys
import os
import subprocess
import traceback
import argparse
import math
from pkg_resources import parse_version
import pandas

if parse_version(pandas.__version__) < parse_version('0.19.2'):
    raise ImportError('Pandas version must be >= v0.19.2!')

import numpy
import code
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
from natsort import natsorted
from cycler import cycler

import geopm_io


class Config(object):
    """The base class for plot configuration objects.

    This class contains the common options for all derived configuration types.

    Attributes:
        profile_name (str): The name used for the title of the plot.
        misc_text (str): Extra text to append to the plot title.
        datatype (str): The datatype to be examined in the plot.
        normalize (bool): A flag controlling whether or not the plot data is normalized.
        output_dir (str): The output directory for storing plots and associated data.
        write_csv (bool): A flag controlling whether or not CSV data for the plots is written.
        output_types (list of str): The list of file formats to be used to save the plots.
        verbose (bool): Print extra information while parsing and plotting if true.
        style (str): The Matplotlib style to use when plotting.
        fig_size (2-tuple of int, overrides base): The (X, Y)size of the plotted figure in inches.
        fontsize (int, overrides base): The size of the font for text in the plot.
        legend_fontsize (int, overrides base): The size of the font in the legend.
        show (bool): Displays an interactive plot if true.
        shell (bool): Drops to a Python shell for further analysis if true.
    """
    def __init__(self, profile_name='Default', misc_text = '', datatype=None, normalize=False,
                 output_dir='figures', write_csv=False, output_types=['svg'], verbose=False,
                 style='classic', fig_size=None, fontsize=None, legend_fontsize=None, show=False, shell=False):
        # Custom params
        self.profile_name = profile_name
        self.misc_text = ' - {}'.format(misc_text) if misc_text else ''
        self.datatype = datatype
        self.normalize = normalize
        self.output_dir = output_dir
        self.write_csv = write_csv
        self.output_types = output_types
        self.verbose = verbose

        # Matplotlib params
        self.fig_size = fig_size
        self.fontsize = fontsize
        self.legend_fontsize = legend_fontsize
        self.show = show
        self.shell = shell

        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)

        if self.show:
            self.block=True

        if self.shell:
            self.show = True
            self.block = False

        plt.style.use(style)
        plt.rcParams.update({'figure.figsize': self.fig_size})
        plt.rcParams.update({'font.size': self.fontsize})


class ReportConfig(Config):
    """ The configuration for plots based on report data.

    This class extends the Config class with the parameters specific to plotting report based data.

    Attributes:
        speedup (bool): Indicates whether or not to plot the target bars as a relative speedup compared
            to the reference bars.
        yspan (int): If speedup is true, the amount of units to include above and below 1.0.
        min_drop (int): The minimum power budget to include in the plotted data.
        max_drop (int): The maximum power budget to include in the plotted data.
        ref_version (str): The reference version string to include in the plotted data.
        ref_profile_name (str): The reference profile name to include in the plotted data.
        ref_plugin (str): The reference plugin to include in the plotted data.
        tgt_version (str): The target version string to include in the plotted data.
        tgt_profile_name (str): The target profile name to include in the plotted data.
        tgt_plugin (str): The target plugin to include in the plotted data.
        datatype (str, overrides base): The desired datatype from the report to plot (e.g. runtime, energy, frequency).
        fig_size (2-tuple of int, overrides base): The (X, Y)size of the plotted figure in inches.
        fontsize (int, overrides base): The size of the font for text in the plot.
        legend_fontsize (int, overrides base): The size of the font in the legend.
        **kwargs: Arbitrary additional overrides for the Config object.
        units (dict of str): The keys are report datatypes and the values are the Y-axis label.
    """
    def __init__(self, speedup=False, yspan = 0.5, min_drop=0, max_drop=999,               # New args for this class
                 ref_version=None, ref_profile_name=None, ref_plugin='static_policy',      # New args for this class
                 tgt_version=None, tgt_profile_name=None, tgt_plugin='power_balancing',    # New args for this class
                 datatype='runtime', fig_size=(6.3, 4.8), fontsize=14, legend_fontsize=14, # Base class args to override
                 **kwargs):                                                                # User overridden args
        super(ReportConfig, self).__init__(datatype=datatype, fig_size=fig_size, fontsize=fontsize,
                                           legend_fontsize=legend_fontsize,
                                           **kwargs)
        self.speedup = speedup
        self.yspan = yspan
        self.min_drop = min_drop
        self.max_drop = max_drop
        self.ref_version = ref_version
        self.ref_profile_name = ref_profile_name
        self.ref_plugin = ref_plugin
        self.tgt_version = tgt_version
        self.tgt_profile_name = tgt_profile_name
        self.tgt_plugin = tgt_plugin
        self.units = {
            'energy': 'J',
            'runtime': 's',
            'frequency': '% of sticker',
        }


class TraceConfig(Config):
    """The configuration for plots based on trace data.

    This class extends the Config class with the parameters specific to plotting trace based data.

    Attributes:
        min_drop (int): The minimum power budget to include in the plotted data.
        max_drop (int): The maximum power budget to include in the plotted data.
        ref_version (str): The reference version string to include in the plotted data.
        ref_profile_name (str): The reference profile name to include in the plotted data.
        ref_plugin (str): The reference plugin to include in the plotted data.
        tgt_version (str): The target version string to include in the plotted data.
        tgt_profile_name (str): The target profile name to include in the plotted data.
        tgt_plugin (str): The target plugin to include in the plotted data.
        legend_label_spacing (float): The spacing between legend labels.
        smooth (int): The number of samples to use in a moving average for the plotted Y-axis data.
        analyze (bool): Flag to control whether basic analysis data is also plotted.
        fig_size (2-tuple of int, overrides base): The (X, Y) size of the plotted figure in inches.
        fontsize (int, overrides base): The size of the font for text in the plot.
        legend_fontsize (int, overrides): The size of the font in the legend.
        **kwargs: Arbitrary additional overrides for the Config object.
    """
    def __init__(self, min_drop=0, max_drop=999,                                        # New args for this class
                 ref_version=None, ref_profile_name=None, ref_plugin='static_policy',   # New args for this class
                 tgt_version=None, tgt_profile_name=None, tgt_plugin='power_balancing', # New args for this class
                 legend_label_spacing = 0.15, smooth=1, analyze=False,                  # New args for this class
                 fig_size=(7, 6), fontsize=16, legend_fontsize=12,                      # Base class args to override
                 **kwargs):                                                             # User overridden args
        super(TraceConfig, self).__init__(fig_size=fig_size, fontsize=fontsize, legend_fontsize=legend_fontsize,
                                           **kwargs)
        self.min_drop = min_drop
        self.max_drop = max_drop
        self.ref_version = ref_version
        self.ref_profile_name = ref_profile_name
        self.ref_plugin = ref_plugin
        self.tgt_version = tgt_version
        self.tgt_profile_name = tgt_profile_name
        self.tgt_plugin = tgt_plugin
        self.legend_label_spacing = legend_label_spacing
        self.smooth = smooth
        self.analyze = analyze

        plt.rcParams.update({'legend.labelspacing': self.legend_label_spacing})

    def get_node_dict(self, node_list):
        """Creates a dictionary of uniform names for node names present in the node list.

        Args:
            node_list (list of str): The list of node names for the current experiment.

        Returns:
            dict: The keys are the experiment node names with the uniform names as values.
        """
        node_list = natsorted(node_list)
        node_dict = {}
        for i, name in enumerate(node_list):
            if self.normalize:
                node_dict[name] = 'Node {}'.format(i + 1)
            else:
                node_dict[name] = name
        return node_dict


def generate_box_plot(report_df, config):
    """Plots boxes for all the input data.

    This will generate a boxplot for every tree decider in the 'report_df' for the target version and profile name.
    It is optionally normalized by the min of the means for the reference profile name, version, and plugin.

    Args:
        report_df (pandas.DataFrame): The multiindexed DataFrame with all the report data parsed from the
            AppOutput class.
        config (ReportConfig): The ReportConfig object specifying the plotting and analysis parameters.

    Todo:
        * Allow for a single plugin to be plotted (e.g. only a target)?
    """
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

    normalization_factor = 1
    if config.normalize:
        # This is the min of the means of all the iterations for datatype per power budget (i.e. rightmost reference bar)
        normalization_factor = report_df.loc[idx[config.ref_version:config.ref_version,
                                                 config.ref_profile_name:config.ref_profile_name,
                                                 config.min_drop:config.max_drop, config.ref_plugin, :, :, :, 'epoch'],
                                             config.datatype].groupby(level='power_budget').mean().min()

    for decider, decider_df in report_df.groupby(level='tree_decider'):
        f, ax = plt.subplots()

        data_df = decider_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                     config.min_drop:config.max_drop, :, :, :, :, 'epoch'], config.datatype]
        data_df /= normalization_factor
        grouped = data_df.groupby(level='power_budget')
        lines = pandas.tools.plotting.boxplot_frame_groupby(grouped, subplots=False, showmeans=True, whis='range',
                                                            ax=ax, return_type='both')

        ylabel = config.datatype.title()
        if config.normalize:
            ylabel = 'Normalized {}'.format(ylabel)
        else:
            units_label = config.units.get(config.datatype)
            ylabel = '{}{}'.format(ylabel, ' ({})'.format(units_label) if units_label else '')
        ax.set_ylabel(ylabel)
        ax.set_xlabel('Per-Node Socket+DRAM Power Limit (W)')

        plt.title('{} {} Boxplot{}'.format(config.profile_name, config.datatype.title(), config.misc_text), y=1.06)
        plt.suptitle(decider.title().replace('_', ' ') + ' Plugin', x=0.54, y=0.90, ha='center')
        plt.margins(0.02, 0.01)
        plt.axis('tight')
        plt.tight_layout()

        # Match the y-axis limits to the bar plot for easier comparison
        ymax = report_df.loc[idx[:, :, config.min_drop:config.max_drop, :, :, :, :, 'epoch'], config.datatype].max()
        ymax /= normalization_factor
        ymax *= 1.1
        ax.set_ylim(0, ymax)

        # Write data/plot files
        file_name = '{}_{}_{}_boxplot'.format(config.profile_name.lower().replace(' ', '_'), config.datatype, decider)
        if config.verbose:
            sys.stdout.write('Writing:\n')
        if config.write_csv:
            full_path = os.path.join(config.output_dir, '{}.csv'.format(file_name))
            grouped.describe().to_csv(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))
        for ext in config.output_types:
            full_path = os.path.join(config.output_dir, '{}.{}'.format(file_name, ext))
            plt.savefig(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        if config.show:
            plt.show(block=config.block)

        if config.shell:
            code.interact(local=dict(globals(), **locals()))


def generate_bar_plot(report_df, config):
    """Plots bars comparing the reference and target plugins.

    This will generate a plot with 2 bars at every power budget in the 'report_df' that compares the reference
    plugin to the target plugin.  This presently is configured only for tree decider comparisons.  The bars are
    optionally normalized by the min of the means of the reference data.

    Args:
        report_df (pandas.DataFrame): The multiindexed DataFrame with all the report data parsed from the
            AppOutput class.
        config (ReportConfig): The config object specifying the plotting and analysis parameters.

    Todo:
        * Allow for a single plugin to be plotted (e.g. only a target)?
    """
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

    decider_list = report_df.index.get_level_values('tree_decider').unique().tolist()
    if config.ref_plugin not in decider_list:
        raise SyntaxError('Reference plugin {} not found in report dataframe!'.format(config.ref_plugin))
    if config.tgt_plugin not in decider_list:
        raise SyntaxError('Target plugin {} not found in report dataframe!'.format(config.tgt_plugin))

    reference_g = report_df.loc[idx[config.ref_version:config.ref_version, config.ref_profile_name:config.ref_profile_name,
                                    config.min_drop:config.max_drop, config.ref_plugin, :, :, :, 'epoch'],
                                config.datatype].groupby(level='power_budget')
    df['reference_mean'] = reference_g.mean()
    df['reference_max'] = reference_g.max()
    df['reference_min'] = reference_g.min()

    target_g = report_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                 config.min_drop:config.max_drop, config.tgt_plugin, :, :, :, 'epoch'],
                             config.datatype].groupby(level='power_budget')
    df['target_mean'] = target_g.mean()
    df['target_max'] = target_g.max()
    df['target_min'] = target_g.min()

    if config.normalize and not config.speedup: # Normalize the data against the rightmost reference bar
        df /= df['reference_mean'].iloc[-1]

    if config.speedup: # Plot the inverse of the target data to show speedup as a positive change
        df = df.div(df['reference_mean'], axis='rows')
        df['target_mean'] = 1 / df['target_mean']
        df['target_max'] = 1 / df['target_max']
        df['target_min'] = 1 / df['target_min']

    # Convert the maxes and mins to be deltas from the mean; required for the errorbar API
    df['reference_max_delta'] = df['reference_max'] - df['reference_mean']
    df['reference_min_delta'] = df['reference_mean'] - df['reference_min']
    df['target_max_delta'] = df['target_max'] - df['target_mean']
    df['target_min_delta'] = df['target_mean'] - df['target_min']

    # Begin plot setup
    f, ax = plt.subplots()
    bar_width = 0.35
    index = numpy.arange(min(len(df['target_mean']), len(df['reference_mean'])))

    plt.bar(index - bar_width / 2,
            df['reference_mean'],
            width=bar_width,
            color='blue',
            align='center',
            label=config.ref_plugin.replace('_', ' ').title(),
            zorder=3)

    ax.errorbar(index - bar_width / 2,
                df['reference_mean'],
                xerr=None,
                yerr=(df['reference_min_delta'], df['reference_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    plt.bar(index + bar_width / 2,
            df['target_mean'],
            width=bar_width,
            color='cyan',
            align='center',
            label=config.tgt_plugin.replace('_', ' ').title(),
            zorder=3) # Forces grid lines to be drawn behind the bar

    ax.errorbar(index + bar_width / 2,
                df['target_mean'],
                xerr=None,
                yerr=(df['target_min_delta'], df['target_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    ax.set_xticks(index)
    ax.set_xticklabels(df.index)
    ax.set_xlabel('Per-Node Socket+DRAM Power Limit (W)')

    ylabel = config.datatype.title()
    if config.normalize and not config.speedup:
        ylabel = 'Normalized {}'.format(ylabel)
    elif not config.normalize and not config.speedup:
        units_label = config.units.get(config.datatype)
        ylabel = '{}{}'.format(ylabel, ' ({})'.format(units_label) if units_label else '')
    else: #if config.speedup:
        ylabel = 'Normalized Speed-up'
    ax.set_ylabel(ylabel)
    ax.grid(axis='y', linestyle='--', color='black')

    plt.title('{} {} Comparison{}'.format(config.profile_name, config.datatype.title(), config.misc_text), y=1.02)
    plt.margins(0.02, 0.01)
    plt.axis('tight')
    plt.legend(shadow=True, fancybox=True, fontsize=config.legend_fontsize, loc='best').set_zorder(11)
    plt.tight_layout()

    if config.speedup:
        ax.set_ylim(1 - config.yspan, 1 + config.yspan)
    else:
        ymax = ax.get_ylim()[1]
        ymax *= 1.1
        ax.set_ylim(0, ymax)

    # Write data/plot files
    file_name = '{}_{}_comparison'.format(config.profile_name.lower().replace(' ', '_'), config.datatype)
    if config.speedup:
        file_name += '_speeudp'
    if config.verbose:
        sys.stdout.write('Writing:\n')
    if config.write_csv:
        full_path = os.path.join(config.output_dir, '{}.csv'.format(file_name))
        df.T.to_csv(full_path)
        if config.verbose:
            sys.stdout.write('    {}\n'.format(full_path))
    for ext in config.output_types:
        full_path = os.path.join(config.output_dir, '{}.{}'.format(file_name, ext))
        plt.savefig(full_path)
        if config.verbose:
            sys.stdout.write('    {}\n'.format(full_path))

    if config.show:
        plt.show(block=config.block)

    if config.shell:
        code.interact(local=dict(globals(), **locals()))


def diff_df(trace_df, column_regex, epoch=True):
    """Diff the DataFrame.

    Since the counters in the trace files are monotonically increasing, a diff must be performed to extract the
    useful data.

    Args:
        trace_df (pandas.DataFrame): The multiindexed DataFrame created by the AppOutput class.
        column_regex (str): A string representing the regex search pattern for the column names to diff.
        epoch (bool): A flag to set whether or not to focus solely on epoch regions.

    Returns:
        pandas.DataFrame: With the diffed columns specified by 'column_regex, and an 'elapsed_time' column.

    Todo:
        * Should I drop everything before the first epoch if 'epoch' is false?
    """
    epoch_rid = '9223372036854775808'

    if epoch:
        tmp_df = trace_df.loc[trace_df['region_id'] == epoch_rid]
    else:
        tmp_df = trace_df

    filtered_df = tmp_df.filter(regex=column_regex)
    filtered_df['elapsed_time'] = tmp_df['seconds']
    filtered_df = filtered_df.diff()
    filtered_df = filtered_df.loc[(filtered_df > 0).all(axis=1)]

    # Reset 'index' to be 0 to the length of the unique trace files
    traces_list = []
    for (version, name, power_budget, tree_decider, leaf_decider, node_name, iteration), df in \
        filtered_df.groupby(level=['version', 'name', 'power_budget', 'tree_decider', 'leaf_decider', 'node_name', 'iteration']):
        df = df.reset_index(level='index')
        df['index'] = pandas.Series(numpy.arange(len(df)), index=df.index)
        df = df.set_index('index', append=True)
        traces_list.append(df)

    return pandas.concat(traces_list)


def get_median_df(diffed_trace_df):
    """Extract the median experiment iteration.

    This logic calculates the sum of elapsed times for all of the experiment iterations for all nodes in
    that iteration.  It then extracts the DataFrame for the iteration that is closest to the median.  For
    input DataFrames with a single iteration, the single iteration is returned.

    Args:
        diffed_trace_df (pandas.DataFrame): The multiindexed DataFrame with 'elapsed_time' calculated in
            diff_df() with 1 or more experiment iterations.

    Returns:
        pandas.DataFrame: Containing a single experiment iteration.
    """
    idx = pandas.IndexSlice
    et_sums = diffed_trace_df.groupby(level=['iteration'])['elapsed_time'].sum()
    median_index = (et_sums - et_sums.median()).abs().sort_values().index[0]
    median_df = diffed_trace_df.loc[idx[:, :, :, :, :, :, median_index],]
    return median_df


def generate_power_plot(trace_df, config):
    """Plots the power consumed per node at each sample.

    This function will plot the power used at each sample for every node.  Specifying the 'analyze' option in the
    config object will also plot the power cap and aggregate power for all nodes.  Specifying the 'normalize' option
    in the config object will use the uniform node names in the plot's legend.

    Args:
        trace_df (pandas.DataFrame): The multiindexed DataFrame with all the trace data parsed from the
            AppOutput class.
        config (TraceConfig): The object specifying the plotting and analysis parameters.

    Raises:
        SyntaxError: If the reference or target plugin was not found in the DataFrame.

    Todo:
        * Resample the median_df to ensure all nodes have the same number of samples.  This can be a source of
            minor error for especially long running apps.
    """
    idx = pandas.IndexSlice
    decider_list = trace_df.index.get_level_values('tree_decider').unique().tolist()
    if config.ref_plugin not in decider_list:
        raise SyntaxError('Reference plugin {} not found in report dataframe!'.format(config.ref_plugin))
    if config.tgt_plugin == 'None': # Allows for plotting all parsed plugins
        config.tgt_plugin = None
    elif config.tgt_plugin not in decider_list:
        raise SyntaxError('Target plugin {} not found in report dataframe!'.format(config.tgt_plugin))

    diffed_df = diff_df(trace_df, 'energy')

    # Calculate power from the diffed counters
    pkg_energy_cols = [s for s in diffed_df.keys() if 'pkg_energy' in s]
    dram_energy_cols = [s for s in diffed_df.keys() if 'dram_energy' in s]
    diffed_df['socket_power'] = diffed_df[pkg_energy_cols].sum(axis=1) / diffed_df['elapsed_time']
    diffed_df['dram_power'] = diffed_df[dram_energy_cols].sum(axis=1) / diffed_df['elapsed_time']
    diffed_df['combined_power'] = diffed_df['socket_power'] + diffed_df['dram_power']

    # Select only the data we care about
    diffed_df = diffed_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                  config.min_drop:config.max_drop, config.tgt_plugin:config.tgt_plugin, :, :, :, :],]

    # Do not include node_name, iteration or index in the groupby clause; The median iteration is extracted and used
    # below for every node togther in a group.  The index must be preserved to ensure the DFs stay in order.
    for (version, name, power_budget, tree_decider, leaf_decider), df in \
        diffed_df.groupby(level=['version', 'name', 'power_budget', 'tree_decider', 'leaf_decider']):

        # Begin plot setup
        node_names = df.index.get_level_values('node_name').unique().tolist()
        node_dict = config.get_node_dict(node_names)
        colors = [plt.get_cmap('plasma')(1. * i/len(node_names)) for i in range(len(node_names))]
        plt.rc('axes', prop_cycle=(cycler('color', colors)))
        f, ax = plt.subplots()

        median_df = get_median_df(df) # Determing the median iteration (if multiple runs)

        if config.verbose:
            median_df_index = []
            median_df_index.append(median_df.index.get_level_values('version').unique()[0])
            median_df_index.append(median_df.index.get_level_values('name').unique()[0])
            median_df_index.append(median_df.index.get_level_values('power_budget').unique()[0])
            median_df_index.append(median_df.index.get_level_values('tree_decider').unique()[0])
            median_df_index.append(median_df.index.get_level_values('leaf_decider').unique()[0])
            median_df_index.append(median_df.index.get_level_values('iteration').unique()[0])

            sys.stdout.write('Plotting {}...\n'.format(' '.join(str(s) for s in median_df_index)))

        for node_name in node_names:
            node_df = median_df.loc[idx[:, :, :, :, :, node_name, :, :],]

            if node_name == 'mr-fusion8':
                plt.plot(pandas.Series(numpy.arange(float(len(node_df))) / (len(node_df) - 1) * 100),
                         node_df['combined_power'].rolling(window=config.smooth, center=True).mean(),
                         label=node_dict[node_name],
                         color='red',
                         path_effects=[pe.Stroke(linewidth=3, foreground='black'), pe.Normal()],
                         zorder=10)
            else:
                plt.plot(pandas.Series(numpy.arange(float(len(node_df))) / (len(node_df) - 1) * 100),
                         node_df['combined_power'].rolling(window=config.smooth, center=True).mean(),
                         label=node_dict[node_name])

        if config.analyze:
            plt.plot(pandas.Series(numpy.arange(float(len(node_df))) / (len(node_df) - 1) * 100),
                     median_df['combined_power'].unstack(level=['node_name']).mean(axis=1),
                     label='Combined Average',
                     color='aqua',
                     linewidth=2.0,
                     path_effects=[pe.Stroke(linewidth=4, foreground='black'), pe.Normal()])
            plt.axhline(power_budget, linewidth=2, color='blue', label='Cap')

        ax.set_xlabel('Iteration # (Normalized)')
        ylabel = 'Socket+DRAM Power (W)'
        if config.smooth > 1:
            ylabel += ' Smoothed'
        ax.set_ylabel(ylabel)

        plt.title('{} Iteration Power\n@ {}W{}'.format(config.profile_name, power_budget, config.misc_text), y=1.02)

        num_nodes = len(node_names)
        if config.analyze:
            num_nodes += 2 # Add 2 node spots for the cap and combined average
        ncol = int(math.ceil(float(num_nodes)/4))

        legend = plt.legend(loc="lower center", bbox_to_anchor=[0.5,0], ncol=ncol,
                            shadow=True, fancybox=True, fontsize=config.legend_fontsize)
        for l in legend.legendHandles:
            l.set_linewidth(2.0)
        legend.set_zorder(11)
        plt.tight_layout()
        ax.set_ylim(ax.get_ylim()[0] * .93, ax.get_ylim()[1])

        # Write data/plot files
        file_name = '{}_power_{}_{}'.format(config.profile_name.lower().replace(' ', '_'), power_budget, tree_decider)
        if config.verbose:
            sys.stdout.write('Writing:\n')

        if config.write_csv:
            full_path = os.path.join(config.output_dir, '{}.csv'.format(file_name))
            median_df.to_csv(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        if config.analyze:
            full_path = os.path.join(config.output_dir, '{}_stats.txt'.format(file_name))
            with open(full_path, 'w') as fd:
                for node_name, node_data in median_df.groupby(level='node_name'):
                    fd.write('{} ({}) statistics -\n\n{}\n\n'.format(node_name, node_dict[node_name], node_data.describe()))
                fd.write('Aggregate (mean) power statistics -\n\n{}'.format(
                         median_df['combined_power'].unstack(level=['node_name']).mean(axis=1).describe()))
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        for ext in config.output_types:
            full_path = os.path.join(config.output_dir, '{}.{}'.format(file_name, ext))
            plt.savefig(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        if config.show:
            plt.show(block=config.block)

        if config.shell:
            code.interact(local=dict(globals(), **locals()))


def generate_epoch_plot(trace_df, config):
    raise NotImplementedError


def generate_freq_plot(trace_df, config):
    raise NotImplementedError


def main(argv):
    trace_plots = {'power', 'epoch', 'freq'}

    _, os.environ['COLUMNS'] = subprocess.check_output(['stty', 'size']).split() # Ensures COLUMNS is set so help text wraps properly
    pandas.set_option('display.width', int(os.environ['COLUMNS']))               # Same tweak for Pandas

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('data_path', metavar='PATH',
                        help='the input path to be searched for report/trace files.',
                        action='store', default='.')
    parser.add_argument('-r', '--report_base',
                        help='the base report string to be searched.',
                        action='store', default='')
    parser.add_argument('-t', '--trace_base',
                        help='the base trace string to be searched.',
                        action='store', default='')
    parser.add_argument('-p', '--plot_types',
                        help='the type of plot to be generated.',
                        action='store', default='bar', type=lambda s : s.split(','))
    parser.add_argument('-s', '--shell',
                        help='drop to a Python shell after plotting.',
                        action='store_true')
    parser.add_argument('-c', '--csv',
                        help='generate CSV files for the plotted data.',
                        action='store_true')
    parser.add_argument('--normalize',
                        help='normalize the data that is plotted',
                        action='store_true')
    parser.add_argument('-o', '--output_types',
                        help='the file type(s) for the plot file output (e.g. svg,png,eps).',
                        action='store', default='svg', type=lambda s : s.split(','))
    parser.add_argument('-n', '--profile_name',
                        help='Name of the profile to be used in file names / plot titles.',
                        action='store')
    parser.add_argument('-m', '--misc_text',
                        help='Text to be appended to the plot title.',
                        action='store', default='')
    parser.add_argument('-v', '--verbose',
                        help='print debugging information.',
                        action='store_true')
    parser.add_argument('--speedup',
                        help='plot the speedup instead of the raw data value.',
                        action='store_true')
    parser.add_argument('--ref_version',
                        help='use this version as the reference to compare against.',
                        action='store', metavar='VERSION')
    parser.add_argument('--ref_profile_name',
                        help='use this name as the reference to compare against.',
                        action='store', metavar='PROFILE_NAME')
    parser.add_argument('--ref_plugin',
                        help='use this tree decider plugin as the reference to compare against.',
                        action='store', default='static_policy', metavar='PLUGIN_NAME')
    parser.add_argument('--tgt_version',
                        help='use this version as the target for analysis (to compare against the ref-plugin.',
                        action='store', metavar='VERSION')
    parser.add_argument('--tgt_profile_name',
                        help='use this name as the target for analysis (to compare against the ref-plugin.',
                        action='store', metavar='PROFILE_NAME')
    parser.add_argument('--tgt_plugin',
                        help='use this tree decider plugin as the target for analysis (to compare against \
                        the ref-plugin).',
                        action='store', default='power_balancing', metavar='PLUGIN_NAME')
    parser.add_argument('--datatype',
                        help='the datatype to be plotted.',
                        action='store', default='runtime')
    parser.add_argument('--smooth',
                        help='apply a NUM_SAMPLES sample moving average to the data plotted on the y axis',
                        action='store', metavar='NUM_SAMPLES', type=int, default=1)
    parser.add_argument('--analyze',
                        help='analyze the data that is plotted',
                        action='store_true')

    args = parser.parse_args(argv)

    if not args.report_base:
        report_glob = '*.report'
    else:
        report_glob = args.report_base + '*'

    if trace_plots.intersection(args.plot_types):
        if not args.trace_base:
            trace_glob = '*.trace-*'
        else:
            trace_glob = args.trace_base + '*'
    else:
        trace_glob = None

    app_output = geopm_io.AppOutput(report_glob, trace_glob, args.data_path, args.verbose)

    if args.profile_name:
        profile_name = args.profile_name
    else:
        profile_name_list = app_output.get_report_df().index.get_level_values('name').unique()
        if len(profile_name_list) > 1:
            raise SyntaxError('Multiple profile names detected! Please provide the -n option to specify the profile name!')
        profile_name = profile_name_list[0]

    report_config = ReportConfig(shell=args.shell, profile_name=profile_name, misc_text=args.misc_text, normalize=args.normalize,
                                 write_csv=args.csv, output_types=args.output_types, verbose=args.verbose,
                                 speedup=args.speedup, datatype=args.datatype,
                                 ref_version=args.ref_version, ref_profile_name=args.ref_profile_name, ref_plugin=args.ref_plugin,
                                 tgt_version=args.tgt_version, tgt_profile_name=args.tgt_profile_name, tgt_plugin=args.tgt_plugin)

    trace_config = TraceConfig(shell=args.shell, profile_name=profile_name, misc_text=args.misc_text, normalize=args.normalize,
                               write_csv=args.csv, output_types=args.output_types, verbose=args.verbose,
                               smooth=args.smooth, analyze=args.analyze,
                               ref_version=args.ref_version, ref_profile_name=args.ref_profile_name, ref_plugin=args.ref_plugin,
                               tgt_version=args.tgt_version, tgt_profile_name=args.tgt_profile_name, tgt_plugin=args.tgt_plugin)

    for plot in args.plot_types:
        plot_func_name = 'generate_{}_plot'.format(plot)
        try:
            if plot in trace_plots:
                globals()[plot_func_name](app_output.get_trace_df(), trace_config)
            else:
                globals()[plot_func_name](app_output.get_report_df(), report_config)
        except KeyError:
            raise KeyError('Invalid plot type "{}"!'.format(plot))

if __name__ == '__main__':
  try:
    main(sys.argv[1:])
  except Exception as e:
    sys.stdout.write('\n   ERROR: {}\n\n'.format(e))
    traceback.print_exc()
    sys.exit(1)

