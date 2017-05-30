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
import cPickle as pickle
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

from geopmpy import __version__
import geopmpy.io

class Config(object):
    """The base class for plot configuration objects.

    This class contains the common options for all derived configuration types.

    Attributes:
        profile_name: The name string used for the title of the plot.
        misc_text: Extra text string to append to the plot title.
        datatype: The string name for a datatype to be examined in the plot.
        normalize: A bool controlling whether or not the plot data is normalized.
        output_dir: The output directory string for storing plots and associated data.
        write_csv: A bool controlling whether or not CSV data for the plots is written.
        output_types: The list of file formats (str) to be used to save the plots.
        verbose: A bool to print extra information while parsing and plotting if true.
        min_drop: The minimum power budget to include in the plotted data.
        max_drop: The maximum power budget to include in the plotted data.
        ref_version: The reference version string to include in the plotted data.
        ref_profile_name: The reference profile name to include in the plotted data.
        ref_plugin: The reference plugin to include in the plotted data.
        tgt_version: The target version string to include in the plotted data.
        tgt_profile_name: The target profile name to include in the plotted data.
        tgt_plugin: The target plugin to include in the plotted data.
        style: The Matplotlib style string to use when plotting.
        fig_size: A 2-tuple of ints for the (X, Y) size of the plotted figure in inches.
        fontsize: The int size of the font for text in the plot.
        legend_fontsize: The int size of the font in the legend.
        show: A bool to display an interactive plot if true.
        shell: A bool to drops to a Python shell for further analysis if true.
    """
    def __init__(self, profile_name='Default', misc_text = '', datatype=None, normalize=False,
                 output_dir='figures', write_csv=False, output_types=['svg'], verbose=False,
                 style='classic', fig_size=None, fontsize=None, legend_fontsize=None, show=False, shell=False,
                 min_drop=0, max_drop=999,
                 ref_version=None, ref_profile_name=None, ref_plugin=None,
                 tgt_version=None, tgt_profile_name=None, tgt_plugin=None):
        # Custom params
        self.profile_name = profile_name
        self.misc_text = ' - {}'.format(misc_text) if misc_text else ''
        self.datatype = datatype
        self.normalize = normalize
        self.output_dir = output_dir
        self.write_csv = write_csv
        self.output_types = output_types
        self.verbose = verbose

        # Indexing params
        self.min_drop = min_drop
        self.max_drop = max_drop
        self.ref_version = ref_version
        self.ref_profile_name = ref_profile_name
        self.ref_plugin = ref_plugin
        self.tgt_version = tgt_version
        self.tgt_profile_name = tgt_profile_name
        self.tgt_plugin = tgt_plugin

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
            self.block = False

        plt.style.use(style)
        plt.rcParams.update({'figure.figsize': self.fig_size})
        plt.rcParams.update({'font.size': self.fontsize})
        plt.rcParams.update({'figure.autolayout': True})

    def check_plugins(self, df, ref_plugin=None, tgt_plugin=None):
        if ref_plugin is None:
            ref_plugin = self.ref_plugin
        if tgt_plugin is None:
            tgt_plugin = self.tgt_plugin

        decider_list = df.index.get_level_values('tree_decider').unique().tolist()
        if ref_plugin is not None and ref_plugin not in decider_list:
            raise LookupError('Reference plugin {} not found in dataframe!'.format(ref_plugin))
        if tgt_plugin is not None and tgt_plugin not in decider_list:
            raise LookupError('Target plugin {} not found in dataframe!'.format(tgt_plugin))


class ReportConfig(Config):
    """The configuration for plots based on report data.

    This class extends the Config class with the parameters specific to plotting report based data.

    Attributes:
        speedup: A bool to indicate whether or not to plot the target bars as a relative speedup compared
            to the reference bars.
        yspan: If speedup is true, the amount of units to include above and below 1.0.
        datatype: The desired datatype from the report to plot (e.g. runtime, energy, frequency).
        fig_size: A 2-tuple of ints for the (X, Y) size of the plotted figure in inches.
        fontsize: The int size of the font for text in the plot.
        legend_fontsize: The size of the font in the legend.
        **kwargs: Arbitrary additional overrides for the Config object.
        units: The keys are report datatypes and the values are the Y-axis label.
    """
    def __init__(self, speedup=False, yspan = 0.35,                                        # New args for this class
                 datatype='runtime', fig_size=(6.3, 4.8), fontsize=14, legend_fontsize=14, # Base class args to override
                 **kwargs):                                                                # User overridden args
        super(ReportConfig, self).__init__(datatype=datatype, fig_size=fig_size, fontsize=fontsize,
                                           legend_fontsize=legend_fontsize, **kwargs)
        self.speedup = speedup
        self.yspan = yspan
        self.units = {
            'energy': 'J',
            'runtime': 's',
            'frequency': '% of sticker',
        }


class TraceConfig(Config):
    """The configuration for plots based on trace data.

    This class extends the Config class with the parameters specific to plotting trace based data.

    Attributes:
        legend_label_spacing: The float spacing between legend labels.
        smooth: The number of samples to use in a moving average for the plotted Y-axis data.
        analyze: Flag to control whether basic analysis data is also plotted.
        base_clock: The base clock frequency for the CPU used in the data.
        focus_node: The node to highlight during per-node plots.
        fig_size: A 2-tuple of ints for the (X, Y) size of the plotted figure in inches.
        fontsize: The int size of the font for text in the plot.
        legend_fontsize: The size of the font in the legend.
        **kwargs: Arbitrary additional overrides for the Config object.
    """
    def __init__(self, legend_label_spacing = 0.15, smooth=1, analyze=False, base_clock=None, # New args for this class
                 focus_node=None,                                                             # New args for this class
                 fig_size=(7, 6), fontsize=16, legend_fontsize=9,                             # Base class args to override
                 **kwargs):                                                                   # User overridden args
        super(TraceConfig, self).__init__(fig_size=fig_size, fontsize=fontsize, legend_fontsize=legend_fontsize,
                                          **kwargs)
        self.legend_label_spacing = legend_label_spacing
        self.smooth = smooth
        self.analyze = analyze
        self.base_clock = base_clock
        self.focus_node = focus_node

        plt.rcParams.update({'legend.labelspacing': self.legend_label_spacing})

    def get_node_dict(self, node_list):
        """Creates a dictionary of uniform names for node names present in the node list.

        Args:
            node_list: The list of node names for the current experiment.

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
        report_df: The multiindexed DataFrame with all the report data parsed from the
            AppOutput class.
        config: The ReportConfig object specifying the plotting and analysis parameters.
    """
    config.check_plugins(report_df)
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

    normalization_factor = 1
    if config.normalize:
        # This is the min of the means of all the iterations for datatype per power budget (i.e. rightmost reference bar)
        normalization_factor = report_df.loc[idx[config.ref_version:config.ref_version,
                                                 config.ref_profile_name:config.ref_profile_name,
                                                 config.min_drop:config.max_drop, config.ref_plugin, :, :, :, 'epoch'],
                                             config.datatype].groupby(level='power_budget').mean().min()

    # Select only the data we care about
    report_df = report_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                config.min_drop:config.max_drop, config.tgt_plugin:config.tgt_plugin],]

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
        plt.suptitle(decider.title().replace('_', ' ') + ' Plugin', x=0.54, y=0.91, ha='center')
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
        sys.stdout.flush()

        if config.show:
            plt.show(block=config.block)

        if config.shell:
            code.interact(local=dict(globals(), **locals()))

        plt.close()


def generate_bar_plot(report_df, config):
    """Plots bars comparing the reference and target plugins.

    This will generate a plot with 2 bars at every power budget in the 'report_df' that compares the reference
    plugin to the target plugin.  This presently is configured only for tree decider comparisons.  The bars are
    optionally normalized by the min of the means of the reference data.

    Args:
        report_df: The multiindexed DataFrame with all the report data parsed from the
            AppOutput class.
        config: The config object specifying the plotting and analysis parameters.

    Todo:
        * Allow for a single plugin to be plotted (e.g. only a target)?
    """
    if config.ref_plugin is None:
        config.ref_plugin = 'static_policy'
        sys.stdout.write('WARNING: No reference plugin set.  Use "--ref_plugin" to override.  ' +
                         'Assuming "static_policy".\n')
    if config.tgt_plugin is None:
        config.tgt_plugin = 'power_balancing'
        sys.stdout.write('WARNING: No target plugin set.  Use "--tgt_plugin" to override.  ' +
                         'Assuming "power_balancing".\n')
    sys.stdout.flush()

    config.check_plugins(report_df)
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

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
        # Check yspan before setting to ensure span is > speedup
        abs_max_val = max(abs(df['target_mean'].max()), abs(df['target_mean'].min()))
        abs_max_val = abs(abs_max_val - 1)
        if abs_max_val  > config.yspan:
            yspan = abs_max_val * 1.1
        else:
            yspan = config.yspan
        ax.set_ylim(1 - yspan, 1 + yspan)
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
    sys.stdout.flush()

    if config.show:
        plt.show(block=config.block)

    if config.shell:
        code.interact(local=dict(globals(), **locals()))

    plt.close()


def generate_power_plot(trace_df, config):
    """Plots the power consumed per node at each sample.

    This function will plot the power used at each sample for every node.  Specifying the 'analyze' option in the
    config object will also plot the power cap and aggregate power for all nodes.  Specifying the 'normalize' option
    in the config object will use the uniform node names in the plot's legend.

    Args:
        trace_df: The multiindexed DataFrame with all the trace data parsed from the
            AppOutput class.
        config: The object specifying the plotting and analysis parameters.

    Raises:
        LookupError: If the reference or target plugin was not found in the DataFrame.

    Todo:
        * Resample the median_df to ensure all nodes have the same number of samples.  This can be a source of
            minor error for especially long running apps.
    """
    config.check_plugins(trace_df)
    idx = pandas.IndexSlice
    decider_list = trace_df.index.get_level_values('tree_decider').unique().tolist()

    # Select only the data we care about
    if config.verbose:
        sys.stdout.write('Filtering data...\n')
        sys.stdout.flush()
    trace_df = trace_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                config.min_drop:config.max_drop, config.tgt_plugin:config.tgt_plugin],]

    if len(trace_df) == 0:
        raise LookupError('No data remains after filtering.  Please check your datasets and filtering options.')

    # Do not include node_name, iteration or index in the groupby clause; The median iteration is extracted and used
    # below for every node togther in a group.  The index must be preserved to ensure the DFs stay in order.
    if config.verbose:
        sys.stdout.write('Grouping data...\n')
        sys.stdout.flush()
    for (version, name, power_budget, tree_decider, leaf_decider), df in \
        trace_df.groupby(level=['version', 'name', 'power_budget', 'tree_decider', 'leaf_decider']):

        # Diff the energy counters and determine the median iteration (if multiple runs)
        median_df = geopmpy.io.Trace.get_median_df(df, 'energy', config)
        # Calculate power from the diffed counters
        pkg_energy_cols = [s for s in median_df.keys() if 'pkg_energy' in s]
        dram_energy_cols = [s for s in median_df.keys() if 'dram_energy' in s]
        median_df['socket_power'] = median_df[pkg_energy_cols].sum(axis=1) / median_df['elapsed_time']
        median_df['dram_power'] = median_df[dram_energy_cols].sum(axis=1) / median_df['elapsed_time']
        median_df['combined_power'] = median_df['socket_power'] + median_df['dram_power']

        # Begin plot setup
        node_names = df.index.get_level_values('node_name').unique().tolist()
        node_dict = config.get_node_dict(node_names)
        colors = [plt.get_cmap('plasma')(1. * i/len(node_names)) for i in range(len(node_names))]
        plt.rc('axes', prop_cycle=(cycler('color', colors)))
        f, ax = plt.subplots()

        for node_name in natsorted(node_names):
            node_df = median_df.loc[idx[:, :, :, :, :, node_name],]

            if node_name == config.focus_node:
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
                     path_effects=[pe.Stroke(linewidth=4, foreground='black'), pe.Normal()],
                     zorder=11)
            plt.axhline(power_budget, linewidth=2, color='blue', label='Cap', zorder=11)

        ax.set_xlabel('Iteration # (Normalized)')
        ylabel = 'Socket+DRAM Power (W)'
        if config.smooth > 1:
            ylabel += ' Smoothed'
        ax.set_ylabel(ylabel)

        plt.title('{} Iteration Power\n@ {}W{}'.format(config.profile_name, power_budget, config.misc_text), y=1.02)

        legend = plt.legend(loc="lower center", bbox_to_anchor=[0.5,0], ncol=4,
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

            full_path = os.path.join(config.output_dir, '{}_mean_node_power.csv'.format(file_name))
            median_df.groupby(level='node_name')['combined_power'].mean().sort_values().to_csv(full_path, header=['combined_power_mean'])
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        if config.analyze:
            full_path = os.path.join(config.output_dir, '{}_stats.txt'.format(file_name))
            with open(full_path, 'w') as fd:
                for node_name, node_data in natsorted(median_df.groupby(level='node_name')):
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
        sys.stdout.flush()

        if config.show:
            plt.show(block=config.block)

        if config.shell:
            code.interact(local=dict(globals(), **locals()))

        plt.close()


def generate_epoch_plot(trace_df, config):
    """Plots the max elapsed time for the nodes at each sample.

    This function will plot the maximum elapsed time for all nodes present in the trace file for each sample.
    Specifying the 'analyze' option till adjust the Y-axis bounds to filter out outliers.  Set '--ref_plugin'
    and '--tgt_plugin' config parameters to the same plugin name to plot a single plugin.

    Args:
        trace_df: The multiindexed DataFrame with all the trace data parsed from the
            AppOutput class.
        config: The object specifying the plotting and analysis parameters.

    Raises:
        LookupError: If the reference or target plugin was not found in the DataFrame.

    Todo:
        * Resample the median_df to ensure all nodes have the same number of samples.  This can be a source of
            minor error for especially long running apps.
    """
    if config.ref_plugin is None:
        ref_plugin = 'static_policy'
        sys.stdout.write('WARNING: No reference plugin set.  Use "--ref_plugin" to override.  ' +
                         'Assuming "static_policy".\n')
    else:
        ref_plugin = config.ref_plugin

    if config.tgt_plugin is None:
        tgt_plugin = 'power_balancing'
        sys.stdout.write('WARNING: No target plugin set.  Use "--tgt_plugin" to override.  ' +
                         'Assuming "power_balancing".\n')
    else:
        tgt_plugin = config.tgt_plugin
    sys.stdout.flush()

    config.check_plugins(trace_df, ref_plugin=ref_plugin, tgt_plugin=tgt_plugin)
    idx = pandas.IndexSlice
    decider_list = trace_df.index.get_level_values('tree_decider').unique().tolist()

    # Select only the data we care about
    if config.verbose:
        sys.stdout.write('Filtering data...\n')
        sys.stdout.flush()
    trace_df = trace_df.loc[idx[config.ref_version:config.tgt_version, config.ref_profile_name:config.tgt_profile_name,
                                config.min_drop:config.max_drop],]

    if len(trace_df) == 0:
        raise LookupError('No data remains after filtering.  Please check your datasets and filtering options.')

    # Group by power budget
    if config.verbose:
        sys.stdout.write('Grouping data...\n')
        sys.stdout.flush()
    for (version, name, power_budget), df in trace_df.groupby(level=['version', 'name', 'power_budget']):
        reference_df = df.loc[idx[config.ref_version:config.ref_version, config.ref_profile_name:config.ref_profile_name,
                                  :, ref_plugin],]
        reference_median_df = geopmpy.io.Trace.get_median_df(reference_df, ' ', config)
        reference_max_time_df = reference_median_df.unstack(level=['node_name']).max(axis=1)

        target_df = df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                               :, tgt_plugin],]
        target_median_df = geopmpy.io.Trace.get_median_df(target_df, ' ', config)
        target_max_time_df = target_median_df.unstack(level=['node_name']).max(axis=1)

        if config.normalize:
            normalization_factor = max(reference_max_time_df.max(), target_max_time_df.max())
            reference_max_time_df /= normalization_factor
            target_max_time_df /= normalization_factor

        f, ax = plt.subplots()
        plt.plot(numpy.arange(float(len(reference_max_time_df))) / len(reference_max_time_df) * 100,
                 reference_max_time_df.rolling(window=config.smooth, center=True).mean(),
                 label=ref_plugin.replace('_', ' ').title(),
                 color='blue',
                 linewidth=1.5)

        if ref_plugin != tgt_plugin: # Do not plot the second line if there is no second plugin
            plt.plot(numpy.arange(float(len(target_max_time_df))) / len(target_max_time_df) * 100,
                     target_max_time_df.rolling(window=config.smooth, center=True).mean(),
                     label=tgt_plugin.replace('_', ' ').title(),
                     color='cyan',
                     linewidth=1.5)

        ax.set_xlabel('Iteration # (Normalized)')
        if config.normalize:
            ylabel = 'Normalized Elapsed Time'
        else:
            ylabel = 'Max Elapsed Time (s)'
        if config.smooth > 1:
            ylabel += ' Smoothed'
        ax.set_ylabel(ylabel)

        plt.title('{} Critical Path Iteration Loop Time\n@ {}W{}'.format(config.profile_name, power_budget,
                                                                         config.misc_text), y=1.02)
        plt.legend(shadow=True, fancybox=True, loc='best', fontsize=14).set_zorder(11)
        plt.tight_layout()

        if config.analyze:
            # Set the ylim from 90% of the 25th percentile to 110% of the 75th percentile
            lower_ylim = min(target_max_time_df.quantile(.25), reference_max_time_df.quantile(.25))
            lower_ylim *= 0.9
            upper_ylim =  max(target_max_time_df.quantile(.75), reference_max_time_df.quantile(.75))
            upper_ylim *= 1.1
            ax.set_ylim(lower_ylim, upper_ylim)

        # Write data/plot files
        file_name = '{}_iteration_loop_time_{}'.format(config.profile_name.lower().replace(' ', '_'), power_budget)
        if config.verbose:
            sys.stdout.write('Writing:\n')

        if config.write_csv:
            full_path = os.path.join(config.output_dir, '{}_reference.csv'.format(file_name))
            reference_median_df.to_csv(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))
            full_path = os.path.join(config.output_dir, '{}_target.csv'.format(file_name))
            target_median_df.to_csv(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        if config.analyze:
            full_path = os.path.join(config.output_dir, '{}_stats.txt'.format(file_name))
            with open(full_path, 'w') as fd:
                fd.write('Reference ({}) time statistics -\n\n{}'.format(ref_plugin,
                        reference_median_df.unstack(level=['node_name']).describe()))
                fd.write('\n\nReference ({}) Aggregate (max) time statistics -\n\n{}'.format(ref_plugin,
                        reference_median_df.unstack(level=['node_name']).mean(axis=1).describe()))
                fd.write('\n\nTarget ({}) time statistics -\n\n{}'.format(tgt_plugin,
                        target_median_df.unstack(level=['node_name']).describe()))
                fd.write('\n\nTarget ({}) Aggregate (max) time statistics -\n\n{}'.format(tgt_plugin,
                         target_median_df.unstack(level=['node_name']).mean(axis=1).describe()))
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))

        for ext in config.output_types:
            full_path = os.path.join(config.output_dir, '{}.{}'.format(file_name, ext))
            plt.savefig(full_path)
            if config.verbose:
                sys.stdout.write('    {}\n'.format(full_path))
        sys.stdout.flush()

        if config.show:
            plt.show(block=config.block)

        if config.shell:
            code.interact(local=dict(globals(), **locals()))

        plt.close()


def generate_freq_plot(trace_df, config):
    """Plots the per sample frequency per node per socket.

    This function will plot the frequency of each socket on each node per sample.  It plots the sockets as seperate
    files denoted '...socket_0.svg', '...socket_1.svg', etc.  Specifying the 'analyze' option in the config object
    will also include a statistics print out of the data used in the plot.  Specifying the 'normalize' option will
    use uniform node names in the plot legend.  Setting the 'config.base_clock' parameter in the config object will
    convert the Y-axis to frequency in GHz as opposed to % of sticker frequency.

    Args:
        trace_df: The multiindexed DataFrame with all the trace data parsed from the
            AppOutput class.
        config: The object specifying the plotting and analysis parameters.

    Raises:
        LookupError: If the reference or target plugin was not found in the DataFrame.

    Todo:
        * Resample the median_df to ensure all nodes have the same number of samples.  This can be a source of
            minor error for especially long running apps.
    """
    config.check_plugins(trace_df)
    idx = pandas.IndexSlice
    decider_list = trace_df.index.get_level_values('tree_decider').unique().tolist()

    # Select only the data we care about
    if config.verbose:
        sys.stdout.write('Filtering data...\n')
        sys.stdout.flush()
    trace_df = trace_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                config.min_drop:config.max_drop, config.tgt_plugin:config.tgt_plugin],]

    if len(trace_df) == 0:
        raise LookupError('No data remains after filtering.  Please check your datasets and filtering options.')

    if config.verbose:
        sys.stdout.write('Grouping data...\n')
        sys.stdout.flush()
    for (version, name, power_budget, tree_decider, leaf_decider), df in \
        trace_df.groupby(level=['version', 'name', 'power_budget', 'tree_decider', 'leaf_decider']):
        # Get the diffed CLK counters, then determine the median iteration (if multiple runs)
        median_df = geopmpy.io.Trace.get_median_df(df, 'clk', config)

        # Begin plot setup
        node_names = df.index.get_level_values('node_name').unique().tolist()
        node_dict = config.get_node_dict(node_names)
        colors = [plt.get_cmap('plasma')(1. * i/len(node_names)) for i in range(len(node_names))]
        plt.rc('axes', prop_cycle=(cycler('color', colors)))
        f, ax = plt.subplots()

        clk_unhalted_core_cols = [s for s in median_df.keys() if 'clk_unhalted_core' in s]
        clk_unhalted_ref_cols = [s for s in median_df.keys() if 'clk_unhalted_ref' in s]

        for c, r in zip(clk_unhalted_core_cols, clk_unhalted_ref_cols): # Loop once per socket
            frequency_data = median_df[c] / median_df[r]
            if config.base_clock:
                frequency_data *= config.base_clock
            else:
                frequency_data *= 100 # Convert from fraction of sticker to % of sticker

            for node_name in natsorted(node_names):
                node_data = frequency_data.loc[idx[:, :, :, :, :, node_name],]

                if node_name == config.focus_node:
                    plt.plot(pandas.Series(numpy.arange(float(len(node_data))) / (len(node_data) - 1) * 100),
                             node_data.rolling(window=config.smooth, center=True).mean(),
                             label=node_dict[node_name],
                             color='red',
                             path_effects=[pe.Stroke(linewidth=3, foreground='black'), pe.Normal()],
                             zorder=10)
                else:
                    plt.plot(pandas.Series(numpy.arange(float(len(node_data))) / (len(node_data) - 1) * 100),
                             node_data.rolling(window=config.smooth, center=True).mean(),
                             label=node_dict[node_name])

            ax.set_xlabel('Iteration # (Normalized)')
            if config.base_clock:
                ylabel = 'Frequency (GHz)'
            else:
                ylabel = '% of Sticker Frequency'
            if config.smooth > 1:
                ylabel += ' Smoothed'
            ax.set_ylabel(ylabel)

            plt.title('{} Iteration Frequency\n@ {}W{}'.format(config.profile_name, power_budget, config.misc_text), y=1.02)

            legend = plt.legend(loc="lower center", bbox_to_anchor=[0.5,0], ncol=4,
                                shadow=True, fancybox=True, fontsize=config.legend_fontsize)
            for l in legend.legendHandles:
                l.set_linewidth(2.0)
            legend.set_zorder(11)
            plt.tight_layout()

            ax.set_ylim(0, ax.get_ylim()[1] * 1.1)

            # Write data/plot files
            file_name = '{}_frequency_{}_{}_socket_{}'.format(config.profile_name.lower().replace(' ', '_'),
                                                              power_budget, tree_decider, clk_unhalted_core_cols.index(c))
            if config.verbose:
                sys.stdout.write('Writing:\n')

            if config.write_csv:
                full_path = os.path.join(config.output_dir, '{}.csv'.format(file_name))
                frequency_data.unstack(level=['node_name']).to_csv(full_path)
                if config.verbose:
                    sys.stdout.write('    {}\n'.format(full_path))

            if config.analyze:
                full_path = os.path.join(config.output_dir, '{}_stats.txt'.format(file_name))
                with open(full_path, 'w') as fd:
                    for node_name, node_data in natsorted(frequency_data.groupby(level='node_name')):
                        fd.write('{} ({}) frequency statistics -\n\n{}\n\n'.format(node_name, node_dict[node_name], node_data.describe()))
                if config.verbose:
                    sys.stdout.write('    {}\n'.format(full_path))

            for ext in config.output_types:
                full_path = os.path.join(config.output_dir, '{}.{}'.format(file_name, ext))
                plt.savefig(full_path)
                if config.verbose:
                    sys.stdout.write('    {}\n'.format(full_path))
            sys.stdout.flush()

            if config.show:
                plt.show(block=config.block)

            if config.shell:
                code.interact(local=dict(globals(), **locals()))

            plt.close()


def main(argv):
    report_plots = {'box', 'bar'}
    trace_plots = {'power', 'epoch', 'freq'}

    _, os.environ['COLUMNS'] = subprocess.check_output(['stty', 'size']).split() # Ensures COLUMNS is set so text wraps
    pandas.set_option('display.width', int(os.environ['COLUMNS']))               # Same tweak for Pandas

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('data_path', metavar='PATH',
                        help='the input path to be searched for report/trace files.',
                        action='store', default='.', nargs='?')
    parser.add_argument('-r', '--report_base',
                        help='the base report string to be searched.',
                        action='store', default='')
    parser.add_argument('-t', '--trace_base',
                        help='the base trace string to be searched.',
                        action='store', default='')
    parser.add_argument('-p', '--plot_types',
                        help='the type of plot to be generated. (e.g. {})'.format(','.join(report_plots | trace_plots)),
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
    parser.add_argument('-o', '--output_dir',
                        help='the output directory for the generated files',
                        action='store', default='figures')
    parser.add_argument('-O', '--output_types',
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
                        action='store', metavar='PLUGIN_NAME')
    parser.add_argument('--tgt_version',
                        help='use this version as the target for analysis (to compare against the ref-plugin.',
                        action='store', metavar='VERSION')
    parser.add_argument('--tgt_profile_name',
                        help='use this name as the target for analysis (to compare against the ref-plugin.',
                        action='store', metavar='PROFILE_NAME')
    parser.add_argument('--tgt_plugin',
                        help='use this tree decider plugin as the target for analysis (to compare against \
                        the ref-plugin).',
                        action='store', metavar='PLUGIN_NAME')
    parser.add_argument('--datatype',
                        help='the datatype to be plotted.',
                        action='store', default='runtime')
    parser.add_argument('--smooth',
                        help='apply a NUM_SAMPLES sample moving average to the data plotted on the y axis',
                        action='store', metavar='NUM_SAMPLES', type=int, default=1)
    parser.add_argument('--analyze',
                        help='analyze the data that is plotted',
                        action='store_true')
    parser.add_argument('--min_drop',
                        help='Minimum power budget to include in the plot.',
                        action='store', metavar='BUDGET_WATTS', type=int, default=-999)
    parser.add_argument('--max_drop',
                        help='Maximum power budget to include in the plot.',
                        action='store', metavar='BUDGET_WATTS', type=int, default=999)
    parser.add_argument('--base_clock',
                        help='Set the base clock frequency (i.e. max non-turbo) for frequency related plots.',
                        action='store', metavar='FREQ_GHZ', type=float)
    parser.add_argument('--focus_node',
                        help='Node to highlight in red during per-node plots.',
                        action='store', metavar='NODE_NAME')
    parser.add_argument('--show',
                        help='show an interactive plot of the data',
                        action='store_true')
    parser.add_argument('--cache',
                        help='Load or save the data parsed in cache files prefixed with FILE_NAME.',
                        action='store', metavar='FILE_NAME')
    parser.add_argument('--version', action='version', version=__version__)

    args = parser.parse_args(argv)

    if report_plots.intersection(args.plot_types):
        if not args.report_base:
            report_glob = '*report'
        else:
            report_glob = args.report_base + '*'
    else:
        report_glob = None

    if trace_plots.intersection(args.plot_types):
        if not args.trace_base:
            trace_glob = '*trace-*'
        else:
            trace_glob = args.trace_base + '*'
    else:
        trace_glob = None

    report_df=None
    trace_df=None
    app_output=None
    if args.cache:
        if args.verbose:
            sys.stdout.write('Trying to load {} caches... '.format(args.cache))
            sys.stdout.flush()
        try:
            report_df = pickle.load(open(args.cache + '_report.p', 'rb'))
            if trace_plots.intersection(args.plot_types):
                trace_df = pickle.load(open(args.cache + '_trace.p', 'rb'))
        except IOError:
            sys.stderr.write('WARNING: File {cache}_report.p or {cache}_trace.p failed to load! '.format(cache=args.cache))
        if args.verbose:
            sys.stdout.write('Done.\n')
            sys.stdout.flush()

    if report_df is None and trace_df is None:
        app_output = geopmpy.io.AppOutput(report_glob, trace_glob, args.data_path, args.verbose)
        if args.cache:
            # Save app_output in cache file
            if args.verbose:
                sys.stdout.write('Saving parsed data to {} cache... '.format(args.cache))
                sys.stdout.flush()
            app_output.get_report_df().to_pickle(args.cache + '_report.p')
            if trace_plots.intersection(args.plot_types):
                app_output.get_trace_df().to_pickle(args.cache + '_trace.p')
            if args.verbose:
                sys.stdout.write('Done.\n')
                sys.stdout.flush()

    if report_df is None:
        report_df = app_output.get_report_df()
    if len(report_df) == 0:
        raise LookupError('No report data in cache.  Tried to load {}_report.p.'.format(args.cache))
    if trace_df is None and trace_plots.intersection(args.plot_types):
        if app_output is None:
            raise LookupError('No trace cache available.  Tried to load {}_trace.p.'.format(args.cache))
        trace_df = app_output.get_trace_df()

    if args.profile_name:
        profile_name = args.profile_name
    else:
        if report_glob is not None:
            profile_name_list = report_df.index.get_level_values('name').unique()
        elif trace_glob is not None:
            profile_name_list = trace_df.index.get_level_values('name').unique()
        else:
            raise LookupError('No glob pattern specified.')

        if len(profile_name_list) > 1:
            raise LookupError('Multiple profile names detected! Please provide the -n option to specify the profile name!')
        profile_name = profile_name_list[0]

    report_config = ReportConfig(shell=args.shell, profile_name=profile_name, misc_text=args.misc_text,
                                 output_dir=args.output_dir, normalize=args.normalize, write_csv=args.csv,
                                 output_types=args.output_types, verbose=args.verbose, speedup=args.speedup,
                                 datatype=args.datatype, min_drop=args.min_drop, max_drop=args.max_drop,
                                 ref_version=args.ref_version, ref_profile_name=args.ref_profile_name,
                                 ref_plugin=args.ref_plugin, tgt_version=args.tgt_version,
                                 tgt_profile_name=args.tgt_profile_name, tgt_plugin=args.tgt_plugin, show=args.show)

    if trace_plots.intersection(args.plot_types):
        trace_config = TraceConfig(shell=args.shell, profile_name=profile_name, misc_text=args.misc_text,
                                   output_dir=args.output_dir, normalize=args.normalize, write_csv=args.csv,
                                   output_types=args.output_types, verbose=args.verbose, smooth=args.smooth,
                                   analyze=args.analyze, min_drop=args.min_drop, max_drop=args.max_drop,
                                   ref_version=args.ref_version, base_clock=args.base_clock,
                                   ref_profile_name=args.ref_profile_name,
                                   ref_plugin=args.ref_plugin, tgt_version=args.tgt_version,
                                   tgt_profile_name=args.tgt_profile_name, tgt_plugin=args.tgt_plugin,
                                   focus_node=args.focus_node, show=args.show)

    for plot in args.plot_types:
        # This tries to create the name of the plot function based on what was parsed in args.plot_types.  If it exists in
        # the global namespace, it can be called through the namespace.
        plot_func_name = 'generate_{}_plot'.format(plot)
        if plot_func_name not in globals():
            raise KeyError('Invalid plot type "{}"!  Valid plots are {}.'.format(plot, ', '.join(plots)))
        if plot in trace_plots:
            if trace_df is None or len(trace_df) == 0:
                raise LookupError('No data present for the requested trace plot.')
            globals()[plot_func_name](trace_df, trace_config)
        else:
            if len(report_df) == 0:
                raise LookupError('No data present for the requested report plot.')
            globals()[plot_func_name](report_df, report_config)

