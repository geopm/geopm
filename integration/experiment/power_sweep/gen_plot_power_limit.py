#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Shows balancer chosen power limits on each socket over time.

'''

import pandas
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
import sys
import os
import argparse

from experiment import common_args
from experiment import plotting

def plot_lines(traces, label, analysis_dir):
    if not os.path.exists(analysis_dir):
        os.mkdir(analysis_dir)

    fig, axs = plt.subplots(2)
    fig.set_size_inches((20, 10))

    num_traces = len(traces)

    colormap = cm.jet
    colors = [colormap(i) for i in np.linspace(0, 1, num_traces*2)]
    idx = 0
    for path in traces:
        node_name = path.split('-')[-1]
        df = pandas.read_csv(path, delimiter='|', comment='#')
        time = df['TIME']
        pl0 = df['MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT-package-0']
        pl1 = df['MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT-package-1']
        rt0 = df['EPOCH_RUNTIME-package-0'] - df['EPOCH_RUNTIME_NETWORK-package-0']
        rt1 = df['EPOCH_RUNTIME-package-1'] - df['EPOCH_RUNTIME_NETWORK-package-1']
        plot_tgt = False
        try:
            tgt = df['POLICY_MAX_EPOCH_RUNTIME']
            plot_tgt = True
        except:
            sys.stdout.write('POLICY_MAX_EPOCH_RUNTIME missing from trace {}; data will be omitted from plot.\n'.format(path))

        color0 = colors[idx]
        color1 = colors[idx + 1]
        idx += 2
        axs[0].plot(time, pl0, color=color0)
        axs[0].plot(time, pl1, color=color1)

        axs[1].plot(time, rt0, label='pkg-0-{}'.format(node_name), color=color0)
        axs[1].plot(time, rt1, label='pkg-1-{}'.format(node_name), color=color1)
    axs[0].set_title('Per socket power limits')
    axs[0].set_ylabel('Power (w)')

    axs[1].set_title('Per socket runtimes and target')
    axs[1].set_xlabel('Time (s)')
    axs[1].set_ylabel('Epoch duration (s)')
    if plot_tgt:
        # draw target once on top of other lines
        axs[1].plot(time, tgt, label='target')
    fig.legend(loc='lower right')
    agent = ' '.join(traces[0].split('_')[1:3]).title()

    fig.suptitle('{} - {}'.format(label, agent), fontsize=20)

    dirname = os.path.dirname(traces[0])
    if len(traces) == 1:
        plot_name = traces[0].split('.')[0] # gadget_power_governor_330_0.trace-epb001
        plot_name += '_' + traces[0].split('-')[1]
    else:
        plot_name = '_'.join(traces[0].split('_')[0:3]) # gadget_power_governor

    outfile = os.path.join(analysis_dir, plot_name + '_power_and_runtime.png')
    sys.stdout.write('Writing {}...\n'.format(outfile))

    fig.savefig(outfile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_output_dir(parser)
    common_args.add_label(parser)
    common_args.add_analysis_dir(parser)

    # Positional arg for gathering all traces into a list
    # Works for files listed explicitly, or with a glob pattern e.g. *trace*
    parser.add_argument('tracepath', metavar='TRACE_PATH', nargs='+',
                        action='store',
                        help='path or glob pattern for trace files to analyze')

    args, _ = parser.parse_known_args()

    # see if paths are valid
    for path in args.tracepath:
        lp = os.path.join(args.output_dir, path)
        if not (os.path.isfile(lp) and os.path.getsize(lp) > 0):
            sys.stderr.write('<geopm> Error: No trace data found in {}\n'.format(lp))
            sys.exit(1)


    plot_lines(args.tracepath, args.label, args.analysis_dir)

