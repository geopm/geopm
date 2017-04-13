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
import fnmatch
from pkg_resources import parse_version
import pandas

if parse_version(pandas.__version__) < parse_version('0.19.2'):
    raise ImportError('Pandas version must be >= v0.19.2!')

import numpy
import code
import matplotlib.pyplot as plt

import geopm_io


class Config(object):
    def __init__(self, app_name='Default', misc_text = '', datatype=None, normalize=False,
                 output_dir='figures', write_csv=False, output_types=['svg'], verbose=False,
                 style='classic', fig_size=None, fontsize=None, legend_fontsize=None, show=False, shell=False):
        # Custom params
        self.app_name = app_name
        self.misc_text = ' - {}'.format(misc_text) if misc_text else ''
        self.datatype = datatype
        self.normalize = normalize
        self.output_dir = output_dir
        self.write_csv = write_csv
        self.output_types = output_types
        self.verbose = verbose
        self.units = {
            'energy': 'J',
            'runtime': 's',
            'frequency': '% of sticker',
        }

        # Matplotlib params
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
        plt.rcParams.update({'figure.figsize': fig_size})
        plt.rcParams.update({'font.size': fontsize})


class ReportConfig(Config):
    def __init__(self, speedup=False, yspan = 0.5, min_drop=0, max_drop=999,               # New args for this class
                 ref_version=None, ref_profile_name=None, ref_plugin='power_governing',    # New args for this class
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


def generate_boxplot(report_df, config):
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

    normalization_factor = 1
    if config.normalize:
        # This is the min of the means of all the iterations for datatype per power budget (i.e. rightmost reference bar)
        normalization_factor = report_df.loc[idx[config.ref_version:config.ref_version,
                                                 config.ref_profile_name:config.ref_profile_name,
                                                 config.min_drop:config.max_drop, :, config.ref_plugin, :, :, 'epoch'],
                                             config.datatype].groupby(level='power_budget').mean().min()

    for decider, decider_df in report_df.groupby(level='leaf_decider'):
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

        plt.title('{} {} Boxplot{}'.format(config.app_name, config.datatype.title(), config.misc_text), y=1.06)
        plt.suptitle(decider.title().replace('_', ' ') + ' Plugin', x=0.54, y=0.90, ha='center')
        plt.margins(0.02, 0.01)
        plt.axis('tight')
        plt.tight_layout()

        # Match the y-axis limits to the bar plot for easier comparison
        ymax = report_df.loc[idx[:, :, config.min_drop:config.max_drop, :, :, :, :, 'epoch'], config.datatype].max()
        ymax /= normalization_factor
        ymax = ymax + ymax * 0.1
        ax.set_ylim(0, ymax)

        # Write data/plot files
        file_name = '{}_{}_{}_boxplot'.format(config.app_name.lower().replace(' ', '_'), config.datatype, decider)
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

def generate_barplot(report_df, config):
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

    decider_list = report_df.index.get_level_values('leaf_decider').unique().tolist()
    if config.ref_plugin not in decider_list:
        raise SyntaxError('Reference plugin {} not found in report dataframe!'.format(config.ref_plugin))
    if config.tgt_plugin not in decider_list:
        raise SyntaxError('Target plugin {} not found in report dataframe!'.format(config.tgt_plugin))

    reference_g = report_df.loc[idx[config.ref_version:config.ref_version, config.ref_profile_name:config.ref_profile_name,
                                    config.min_drop:config.max_drop, :, config.ref_plugin, :, :, 'epoch'],
                                config.datatype].groupby(level='power_budget')
    df['reference_mean'] = reference_g.mean()
    df['reference_max'] = reference_g.max()
    df['reference_min'] = reference_g.min()

    target_g = report_df.loc[idx[config.tgt_version:config.tgt_version, config.tgt_profile_name:config.tgt_profile_name,
                                 config.min_drop:config.max_drop, :, config.tgt_plugin, :, :, 'epoch'],
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

    plt.title('{} {} Comparison{}'.format(config.app_name, config.datatype.title(), config.misc_text), y=1.02)
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
    file_name = '{}_{}_comparison'.format(config.app_name.lower().replace(' ', '_'), config.datatype)
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

def main(argv):

    plots = {
        'barplot' : generate_barplot,
        'boxplot' : generate_boxplot,
    }

    _, os.environ['COLUMNS'] = subprocess.check_output(['stty', 'size']).split() # Ensures COLUMNS is set so help text wraps properly
    pandas.set_option('display.width', int(os.environ['COLUMNS']))               # Same tweak for Pandas

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('data_path', metavar='PATH',
                        help='the input path to be searched for report/trace files.',
                        action='store')
    parser.add_argument('-r', '--report_base',
                        help='the base report string to be searched.',
                        action='store', default='')
    parser.add_argument('-t', '--trace_base',
                        help='the base trace string to be searched.',
                        action='store', default='')
    parser.add_argument('-p', '--plot_types',
                        help='the type of plot to be generated.',
                        action='store', default='barplot', type=lambda s : s.split(','))
    parser.add_argument('-s', '--shell',
                        help='drop to a Python shell after plotting.',
                        action='store_true')
    parser.add_argument('-c', '--csv',
                        help='generate CSV files for the plotted data.',
                        action='store_true')
    parser.add_argument('-n', '--normalize',
                        help='normalize the data that is plotted',
                        action='store_true')
    parser.add_argument('-o', '--output_types',
                        help='the file type(s) for the plot file output (e.g. svg,png,eps).',
                        action='store', default='svg', type=lambda s : s.split(','))
    parser.add_argument('-a', '--app_name',
                        help='Name of the app to be used in file names / plot titles.',
                        action='store')
    parser.add_argument('-m', '--misc_text',
                        help='Text to be appended to the plot title.',
                        action='store', default='')
    parser.add_argument('-v', '--verbose',
                        help='print debugging information.',
                        action='store_true')
    parser.add_argument('--speedup',
                        help='barplots: plot the speedup instead of the raw data value.',
                        action='store_true')
    parser.add_argument('--ref_version',
                        help='barplots: use this version as the reference to compare against.',
                        action='store', metavar='VERSION')
    parser.add_argument('--ref_profile_name',
                        help='barplots: use this name as the reference to compare against.',
                        action='store', metavar='PROFILE_NAME')
    parser.add_argument('--ref_plugin',
                        help='barplots: use this leaf decider plugin as the reference to compare against.',
                        action='store', default='power_governing', metavar='PLUGIN_NAME')
    parser.add_argument('--tgt_version',
                        help='barplots: use this version as the target for analysis (to compare against the ref-plugin.',
                        action='store', metavar='VERSION')
    parser.add_argument('--tgt_profile_name',
                        help='barplots: use this name as the target for analysis (to compare against the ref-plugin.',
                        action='store', metavar='PROFILE_NAME')
    parser.add_argument('--tgt_plugin',
                        help='barplots: use this leaf decider plugin as the target for analysis (to compare against \
                        the ref-plugin).',
                        action='store', default='power_balancing', metavar='PLUGIN_NAME')
    parser.add_argument('--datatype',
                        help='the datatype to be plotted.',
                        action='store', default='runtime')

    args = parser.parse_args(argv)

    ao = geopm_io.AppOutput(os.path.join(args.data_path, args.report_base))
    app_name = args.app_name if args.app_name else ao.get_report_df().index.get_level_values('name').unique()[0]
    cg = ReportConfig(shell=args.shell, app_name=app_name, misc_text=args.misc_text, normalize=args.normalize,
                      write_csv=args.csv, output_types=args.output_types, verbose=args.verbose,
                      speedup=args.speedup, datatype=args.datatype,
                      ref_version=args.ref_version, ref_profile_name=args.ref_profile_name, ref_plugin=args.ref_plugin,
                      tgt_version=args.tgt_version, tgt_profile_name=args.tgt_profile_name, tgt_plugin=args.tgt_plugin)

    for plot in args.plot_types:
        if plot not in plots:
            raise KeyError('Invalid plot type "{}"!'.format(plot))
        plots[plot](ao.get_report_df(), cg)


if __name__ == '__main__':
  try:
    main(sys.argv[1:])
  except Exception as e:
    sys.stdout.write('\n   ERROR: {}\n\n'.format(e))
    traceback.print_exc()
    sys.exit(1)

