#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Displays a histogram of the control loop sample intervals
'''

import pandas
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
import sys
import os


def rate_hist(traces):
    num_traces = len(traces)
    all_delta_t = pandas.DataFrame()
    for path in traces:
        df = pandas.read_csv(path, delimiter='|', comment='#')
        delta_t = df['TIME'].diff()[1:]
        delta_t = delta_t.loc[delta_t != 0]
        all_delta_t = all_delta_t.append(delta_t, ignore_index=True)

    plt.hist(all_delta_t, 100)

    dir_name = os.path.dirname(traces[0])
    plt.title('Control Loop Duration {}'.format(dir_name))
    plt.ylabel('count')
    plt.xlabel('sample duration (sec)')
    plt.savefig(os.path.join(dir_name, 'rate_hist.png'))

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stderr.write('Provide paths to trace file(s)')
        sys.exit(1)
    rate_hist(sys.argv[1:])
