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
import pandas
import numpy
import code
import matplotlib.pyplot as plt

import geopm_io


class Config(object):
    def __init__(self, name='Default', misc_text = '', datatype=None, external=False,
                 output_dir='figures', write_csv=False, output_types=['svg'], verbose=False,
                 style='classic', fig_size=None, fontsize=None, legend_fontsize = None, show=False, shell=False):
        # Custom params
        self.name = name
        self.misc_text = ' - {}'.format(misc_text) if misc_text else misc_text
        self.datatype = datatype
        self.external = external
        self.output_dir = output_dir
        self.write_csv = write_csv
        self.output_types = output_types
        self.verbose = verbose
        self.units = {
            'energy': '(J)',
            'runtime': '(s)',
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
                 datatype='runtime', fig_size=(6.3, 4.8), fontsize=14, legend_fontsize=14, # Base class args to override
                 **kwargs):                                                                # User overridden args
        super(ReportConfig, self).__init__(datatype=datatype, fig_size=fig_size, fontsize=fontsize,
                                           legend_fontsize=legend_fontsize,
                                           **kwargs)
        self.speedup = speedup
        self.yspan = yspan
        self.min_drop = min_drop
        self.max_drop = max_drop


def generate_boxplot(report_df, config):
    idx = pandas.IndexSlice
    df = pandas.DataFrame()

    normalization_factor = 1
    if config.external:
        # This is the min of the means of all the iterations for datatype per power budget (i.e. rightmost governing bar)
        normalization_factor = report_df.loc[idx[config.min_drop:config.max_drop, :, 'power_governing', :, :, 'epoch'],
                                                 config.datatype].groupby(level='power_budget').mean().min()

    for decider, decider_df in report_df.groupby(level='leaf_decider'):
        f, ax = plt.subplots()

        data_df = decider_df.loc[idx[config.min_drop:config.max_drop, :, :, :, :, 'epoch'], config.datatype]
        data_df /= normalization_factor
        grouped = data_df.groupby(level='power_budget')
        lines = pandas.tools.plotting.boxplot_frame_groupby(grouped, subplots=False, showmeans=True, whis='range',
                                                            ax=ax, return_type='both')

        ylabel = config.datatype.title()
        if config.external:
            ylabel = 'Normalized {}'.format(ylabel)
        else:
            units_label = config.units.get(config.datatype)
            ylabel = '{}{}'.format(ylabel, ' {}'.format(units_label) if units_label else '')
        ax.set_ylabel(ylabel)
        ax.set_xlabel('Per-Node Socket+DRAM Power Limit (W)')

        plt.title('{} {} Boxplot{}'.format(config.name, config.datatype.title(), config.misc_text), y=1.06)
        plt.suptitle(decider.title().replace('_', ' ') + ' Plugin', x=0.54, y=0.90, ha='center')
        plt.margins(0.02, 0.01)
        plt.axis('tight')
        plt.tight_layout()

        # Match the y-axis limits to the bar plot for easier comparison
        ymax = report_df.loc[idx[config.min_drop:config.max_drop, :, :, :, :, 'epoch'], config.datatype].max()
        ymax = ymax + ymax * 0.1
        ax.set_ylim(0, ymax)

        # Write data/plot files
        file_name = '{}_{}_{}_boxplot'.format(config.name.lower().replace(' ', '_'), config.datatype, decider)
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

    governed_g = report_df.loc[idx[:, :, 'power_governing', :, :, 'epoch'], config.datatype].groupby(level='power_budget')
    df['governed_mean'] = governed_g.mean()
    df['governed_max'] = governed_g.max()
    df['governed_min'] = governed_g.min()

    balanced_g = report_df.loc[idx[:, :, 'power_balancing', :, :, 'epoch'], config.datatype].groupby(level='power_budget')
    df['balanced_mean'] = balanced_g.mean()
    df['balanced_max'] = balanced_g.max()
    df['balanced_min'] = balanced_g.min()

    if config.external and not config.speedup: # Normalize the data against the rightmost governed bar
        df /= df['governed_mean'].iloc[-1]

    if config.speedup: # Plot the inverse of the balanced data to show speedup as a positive change
        df = df.div(df['governed_mean'], axis='rows')
        df['balanced_mean'] = 1 / df['balanced_mean']
        df['balanced_max'] = 1 / df['balanced_max']
        df['balanced_min'] = 1 / df['balanced_min']

    # Convert the maxes and mins to be deltas from the mean; required for the errorbar API
    df['governed_max_delta'] = numpy.abs(df['governed_max'] - df['governed_mean'])
    df['governed_min_delta'] = numpy.abs(df['governed_min'] - df['governed_mean'])
    df['balanced_max_delta'] = numpy.abs(df['balanced_max'] - df['balanced_mean'])
    df['balanced_min_delta'] = numpy.abs(df['balanced_min'] - df['balanced_mean'])

    # Begin plot setup
    f, ax = plt.subplots()
    bar_width = 0.35
    index = numpy.arange(min(len(df['balanced_mean']), len(df['governed_mean'])))

    plt.bar(index + bar_width / 2,
            df['balanced_mean'],
            width=bar_width,
            color='cyan',
            align='center',
            label='Balancer Plugin',
            zorder=3) # Forces grid lines to be drawn behind the bar

    ax.errorbar(index + bar_width / 2,
                df['balanced_mean'],
                xerr=None,
                yerr=(df['balanced_min_delta'], df['balanced_max_delta']),
                fmt=' ',
                label='',
                color='r',
                elinewidth=2,
                capthick=2,
                zorder=10)

    plt.bar(index - bar_width / 2,
            df['governed_mean'],
            width=bar_width,
            color='blue',
            align='center',
            label='Baseline',
            zorder=3)

    ax.errorbar(index - bar_width / 2,
                df['governed_mean'],
                xerr=None,
                yerr=(df['governed_min_delta'], df['governed_max_delta']),
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
    if config.external and not config.speedup:
        ylabel = 'Normalized {}'.format(ylabel)
    elif not config.external and not config.speedup:
        units_label = config.units.get(config.datatype)
        ylabel = '{}{}'.format(ylabel, ' {}'.format(units_label) if units_label else '')
    else: #if config.speedup:
        ylabel = 'Normalized Speed-up'
    ax.set_ylabel(ylabel)
    ax.grid(axis='y', linestyle='--', color='black')

    plt.title('{} {} Comparison{}'.format(config.name, config.datatype.title(), config.misc_text), y=1.02)
    plt.margins(0.02, 0.01)
    plt.axis('tight')
    plt.legend(shadow=True, fancybox=True, fontsize=config.legend_fontsize, loc='best').set_zorder(11)
    plt.tight_layout()

    if config.speedup:
        ax.set_ylim(1 - config.yspan, 1 + config.yspan)
    else:
        ymax = ax.get_ylim()[1]
        ymax = ymax + ymax * 0.1
        ax.set_ylim(0, ymax)

    # Write data/plot files
    file_name = '{}_{}_comparison'.format(config.name.lower().replace(' ', '_'), config.datatype)
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
                        action='store', default='*report*')
    parser.add_argument('-t', '--trace_base',
                        help='the base trace string to be searched.',
                        action='store', default='*trace*')
    parser.add_argument('-p', '--plot_type',
                        help='the type of plot to be generated',
                        action='store', default='barplot', choices=plots.keys())
    parser.add_argument('-s', '--shell',
                        help='drop to a shell after plotting',
                        action='store_true')
    parser.add_argument('-c', '--csv',
                        help='generate CSV files for the plotted data',
                        action='store_true')
    parser.add_argument('-e', '--external',
                        help='format the plots for external release',
                        action='store_true')
    parser.add_argument('-o', '--output_types',
                        help='the file type(s) for the plot file output (e.g. svg,png,eps)',
                        action='store', default='svg', type=lambda s : s.split(','))
    parser.add_argument('-a', '--app_name',
                        help='Name of the app to be used in file names / plot titles.',
                        action='store', default='Test Data')
    parser.add_argument('-m', '--misc_text',
                        help='Text to be appended to the plot title.',
                        action='store', default='Knights Landing')
    parser.add_argument('-v', '--verbose',
                        help='print debugging information',
                        action='store_true')
    parser.add_argument('--speedup',
                        help='barplots: plot the speedup instead of the raw data value',
                        action='store_true')
    parser.add_argument('--datatype',
                        help='the datatype to be plotted',
                        action='store', default='runtime')

    args = parser.parse_args(argv)

    ao = geopm_io.AppOutput(os.path.join(args.data_path, args.report_base))
    cg = ReportConfig(shell=args.shell, name=args.app_name, misc_text=args.misc_text, external=args.external,
                write_csv=args.csv, output_types=args.output_types, verbose=args.verbose,
                speedup=args.speedup, datatype=args.datatype)

    plots[args.plot_type](ao.get_report_df(), cg)


if __name__ == '__main__':
  try:
    main(sys.argv[1:])
  except Exception, e:
    print
    print "   ERROR: %s" % (e,)
    print
    traceback.print_exc()
    sys.exit(1)
