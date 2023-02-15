#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import pandas
import numpy as np
import glob
from collections import defaultdict
import sys
import math

import geopmpy.io

# TODO: copied from plotter.py
# maybe move these functions into that file
import subprocess
import os
try:
    with open(os.devnull, 'w') as FNULL:
        subprocess.check_call("python3 -c 'import matplotlib.pyplot'", stdout=FNULL, stderr=FNULL, shell=True)
except subprocess.CalledProcessError:
    sys.stderr.write('Warning: Unable to use default matplotlib backend ({}).  For interactive plotting,'
                     ' please install Tkinter support in the OS.  '
                     'For more information see: https://matplotlib.org/faq/usage_faq.html#what-is-a-backend\n'
                     'Trying Agg...\n\n'
                     .format(os.getenv('MPLBACKEND', 'TkAgg')))
    import matplotlib
    matplotlib.use('Agg')

import matplotlib.pyplot as plt
from matplotlib import cm, colors


class ShortRegionPerfMarginAnalysis:

    @staticmethod
    def profile_to_big_o(df):
        '''
        Used to extract big-o from the profile name and rename profile to big-o only
        '''
        def extract_big_o(profile_str):
            return float(profile_str.split('_')[-3])

        df['Profile'] = df['Profile'].apply(extract_big_o)
        return df

    @staticmethod
    def extract_bigo_from_region(df):
        '''
        Assumes the bigo is on the end of the region name.  Adds a new column
        for the bigo and renames region to the base region.
        TODO: some interference from auto regions (OMPT)
        '''
        def extract_big_o(region_str):
            if '_' not in region_str or 'OMPT' in region_str:
                return float('nan')
            return float(region_str.split('_')[-1])

        def extract_region(region_str):
            if '_' not in region_str or 'OMPT' in region_str:
                return region_str
            return '_'.join(region_str.split('_')[:-1])

        bigos = df['region'].apply(extract_big_o)
        bigos = bigos.rename('bigo')
        df['region'] = df['region'].apply(extract_region)
        df = pandas.concat([df, bigos], axis=1)
        return df

    def __init__(self, outdir='.'):
        self._min_freq = 1000000000  # TODO: use geopmread
        self._max_freq = 2100000000  # TODO: use geopmread
        self._step_freq = 100000000
        # TODO: max is baseline for comparison

        self._outdir = outdir

    # TODO: select by multiple column values at once
    def get_baseline_data(self, df):
        baseline_df = df.loc[(df['FREQ_MIN'] == self._max_freq) & (df['FREQ_MAX'] == self._max_freq)]
        return baseline_df

    def get_learning_data(self, df):
        learning_df = df.loc[(df['FREQ_MIN'] == self._min_freq) & (df['FREQ_MAX'] == self._max_freq)]
        return learning_df

    # TODO: static methods might be nice as helpers in io.py
    @staticmethod
    def get_column_values(df, col_name):
        rv = df[col_name].dropna().unique()
        return sorted(rv)

    @staticmethod
    def filter_by_column_value(df, col_name, col_value):
        df = df.loc[df[col_name] == col_value]
        return df

    def plot_selected_frequencies(self, df, title_label):
        '''
        Plot with region runtime and perf margin vs selected frequency

        TODO: move to plotter?

        Input should be filtered to the agent+policy of interest (learning runs)
        and region of interest
        '''
        fdf = self.get_learning_data(df)

        big_os = self.get_column_values(fdf, 'bigo')
        perf_margins = self.get_column_values(fdf, 'PERF_MARGIN')

        selected = []
        selected_error = []
        for bigo in big_os:
            tmp = []
            etmp = []
            for pp in perf_margins:
                rpdf = self.filter_by_column_value(fdf, 'bigo', bigo)
                rpdf = self.filter_by_column_value(rpdf, 'PERF_MARGIN', pp)
                freq = rpdf['requested-online-frequency'].mean()
                freq = freq / 1e9
                error = rpdf['requested-online-frequency'].std()
                error = error / 1e9
                tmp.append(freq)
                etmp.append(error)
            selected.append(tmp)
            selected_error.append(etmp)
        ## TODO: need to handle NAN somehow
        # NAN means EE agent failed to learn.

        # clean up long floats
        perf_margins = ['{:.3f}'.format(yy) for yy in perf_margins]
        # update xtick labels to be scientific notation
        big_os = ['$2^{{{}}}$'.format(int(math.log2(xx*10000))) for xx in big_os]
        x_label = 'big-o (100us = $10^{{-4}}$s)'

        cmap, norm, freqs, z_thresh = self.frequency_colormap()
        self.plot_heatmap(data=selected, cmap=cmap, norm=norm, zbar_range=freqs, z_thresh=z_thresh,
                          zbar_label='selected frequency (GHz)',
                          x_range=big_os, x_label=x_label,
                          y_range=perf_margins, y_label='perf margin (max % degradation)',
                          title='EE Agent Selected Frequency' + title_label,
                          outdir=self._outdir)
        cmap, norm, freqs, z_thresh = self.frequency_error_colormap()
        self.plot_heatmap(data=selected_error, cmap=cmap, norm=norm, zbar_range=freqs, z_thresh=z_thresh,
                          zbar_label='stdev selected frequency (GHz)',
                          x_range=big_os, x_label=x_label,
                          y_range=perf_margins, y_label='perf margin (max % degradation)',
                          title='EE Agent Std Dev Selected Frequency' + title_label,
                          outdir=self._outdir)

    def frequency_colormap(self):
        freqs = np.arange(self._min_freq, self._max_freq + self._step_freq, self._step_freq)
        freqs = [xx / 1e9 for xx in freqs]
        z_thresh = 1.7
        cmap = cm.get_cmap('magma', len(freqs))
        norm = colors.BoundaryNorm(freqs, cmap.N)
        return cmap, norm, freqs, z_thresh

    def frequency_error_colormap(self):
        freqs = np.linspace(0.0, self._step_freq * 5, 10)
        freqs = [xx / 1e9 for xx in freqs]
        z_thresh = 0.5
        cmap = cm.get_cmap('magma', len(freqs))
        norm = colors.BoundaryNorm(freqs, cmap.N)
        return cmap, norm, freqs, z_thresh

    @staticmethod
    def comparison_3d(base_df, comp_df, x_dim, y_dim, z_dim, baseline_y=False):
        xvals = ShortRegionPerfMarginAnalysis.get_column_values(comp_df, x_dim)
        yvals = ShortRegionPerfMarginAnalysis.get_column_values(comp_df, y_dim)
        data = []
        for xx in xvals:
            tmp = []
            for yy in yvals:
                base = ShortRegionPerfMarginAnalysis.filter_by_column_value(base_df, x_dim, xx)
                comp = ShortRegionPerfMarginAnalysis.filter_by_column_value(comp_df, x_dim, xx)
                # TODO: is baseline also swept over both dims?
                if baseline_y:
                    base = ShortRegionPerfMarginAnalysis.filter_by_column_value(base, y_dim, yy)
                comp = ShortRegionPerfMarginAnalysis.filter_by_column_value(comp, y_dim, yy)
                # TODO: have a separate heatmap with error
                rs = (base[z_dim].mean() - comp[z_dim].mean()) / base[z_dim].mean() * 100
                tmp.append(rs.mean())
            data.append(tmp)
        return xvals, yvals, data

    def plot_energy_savings(self, base_df, comp_df, title_label):
        big_os, perf_margins, savings = self.comparison_3d(base_df, comp_df, 'bigo', 'PERF_MARGIN', 'package-energy (joules)')

        # clean up long floats
        perf_margins = ['{:.3f}'.format(yy) for yy in perf_margins]
        # update xtick labels to be scientific notation
        big_os = ['$2^{{{}}}$'.format(int(math.log2(xx*10000))) for xx in big_os]
        x_label = 'big-o (100us = $10^{{-4}}$s)'

        cmap, norm, percents, z_thresh = self.energy_savings_colormap()
        self.plot_heatmap(data=savings, cmap=cmap, norm=norm, zbar_range=percents, z_thresh=z_thresh,
                          zbar_label='% energy savings vs. sticker',
                          x_range=big_os, x_label=x_label,
                          y_range=perf_margins, y_label='perf margin (max % degradation)',
                          title='Energy Savings' + title_label, outdir=self._outdir)

    def energy_savings_colormap(self):
        percents = np.linspace(0.0, 50.0, 20)
        z_thresh = 0.0
        cmap = cm.get_cmap('autumn')
        norm = colors.Normalize(percents.min(), percents.max())
        return cmap, norm, percents, z_thresh

    def plot_performance_degradation(self, base_df, comp_df, title_label):
        big_os, perf_margins, savings = self.comparison_3d(base_df, comp_df, 'bigo', 'PERF_MARGIN', 'runtime (sec)')
        # clean up long floats
        perf_margins = ['{:.3f}'.format(yy) for yy in perf_margins]
        # update xtick labels to be scientific notation
        big_os = ['$2^{{{}}}$'.format(int(math.log2(xx*10000))) for xx in big_os]
        x_label = 'big-o (100us = $10^{{-4}}$s)'

        cmap, norm, percents, z_thresh = self.runtime_degradation_colormap()
        self.plot_heatmap(data=savings, cmap=cmap, norm=norm, zbar_range=percents, z_thresh=z_thresh,
                          zbar_label='% runtime degradation vs. sticker',
                          x_range=big_os, x_label=x_label,
                          y_range=perf_margins, y_label='perf margin (max % degradation)',
                          title='Runtime Degradation' + title_label, outdir=self._outdir)

    def runtime_degradation_colormap(self):
        ''' Pick range of values, colormap, text color threshold that look good for data'''
        percents = np.linspace(-20.0, 20.0, 20)
        z_thresh = -14.0
        cmap = cm.get_cmap('coolwarm')
        norm = colors.Normalize(percents.min(), percents.max())
        return cmap, norm, percents, z_thresh

    @staticmethod
    def plot_heatmap(data, cmap, norm, zbar_range, zbar_label, z_thresh,
                     x_range, x_label, y_range, y_label, title, outdir):

        # need to transpose the data for imshow
        data = np.array(data)
        data = data.T

        f, ax = plt.subplots()
        im = ax.imshow(data, interpolation='none', cmap=cmap, norm=norm)
        cbar = ax.figure.colorbar(im, ax=ax, ticks=zbar_range)
        cbar.ax.set_ylabel(zbar_label, rotation=-90, va='bottom')

        ax.set_xlabel(x_label)
        ax.set_xticklabels(x_range)
        ax.set_xticks(np.arange(len(x_range)))

        ax.set_ylabel(y_label)
        ax.set_yticklabels(y_range)
        ax.set_yticks(np.arange(len(y_range)))

        # display text value over color
        for x in range(len(x_range)):
            for y in range(len(y_range)):
                # note reversal of x and y for data access
                color = 'black'
                if data[y][x] < z_thresh:
                    color = 'white'
                ax.text(x, y, '{:.2f}'.format(data[y][x]), ha='center', va='center',
                        color=color, size=8)

        ######################
        # fix for top and bottom being cut off
        b, t = ax.get_ylim()
        b += 0.5
        t -= 0.5
        ax.set_ylim(b, t)
        ################

        plt.title(title)
        f.tight_layout()
        filename = '{}.png'.format(title.replace(' ', '_').replace('\n', '_'))
        plt.savefig(os.path.join(outdir, filename))
        plt.close()


if __name__ == '__main__':
    # get reports location from command line for now
    if len(sys.argv) < 2:
        sys.stderr.write('Pass path to reports.\n')
        sys.exit(1)
    outdir = sys.argv[1]

    collection = geopmpy.io.RawReportCollection('*.report', dir_name=outdir)
    df = collection.get_df()

    # rename region and extract big-o
    df = ShortRegionPerfMarginAnalysis.extract_bigo_from_region(df)

    # TODO: this requires exact match currently but could take a pattern or substring
    region_name = 'timed_scaling'
    df = ShortRegionPerfMarginAnalysis.filter_by_column_value(df, 'region', region_name)

    main = ShortRegionPerfMarginAnalysis(outdir=outdir)

    region_label = ':\n{} region'.format(region_name)
    main.plot_selected_frequencies(df, region_label)

    base_df = main.get_baseline_data(df)
    comp_df = main.get_learning_data(df)
    main.plot_energy_savings(base_df, comp_df, region_label)
    main.plot_performance_degradation(base_df, comp_df, region_label)
