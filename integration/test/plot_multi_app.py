#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import os
from geopmpy.io import RawReportCollection as RR
import matplotlib.pyplot as plt


def main():
    allr = sys.argv[1:]
    data = [RR(nn) for nn in allr]
    data_map = {len(dd.get_app_df()): dd for dd in data}
    nodec = sorted(data_map.keys())
    startup = [data_map[nc].get_app_df()['GEOPM startup (s)']
               for nc in nodec]
    overhead = [data_map[nc].get_app_df()['GEOPM overhead (s)']
                for nc in nodec]
    make_plot(nodec, startup, overhead, 'GEOPM-overhead.png')

def make_plot(nodec, startup, overhead, plot_path):
    labels = [str(nc) for nc in nodec]
    plt.subplot(2, 1, 1)
    plt.boxplot(overhead, labels=labels)
    plt.title('GEOPM Overheads')
    plt.ylabel('Application Extra Time (s)')
    plt.subplot(2, 1, 2)
    plt.boxplot(startup, labels=labels)
    plt.ylabel('Runtime Startup Time (s)')
    plt.xlabel('Node Count')
    plt.savefig(plot_path)

if __name__ == '__main__':
    main()
