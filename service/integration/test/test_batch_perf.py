#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import matplotlib.pyplot as plt

def main():
    cases = ['msr-safe-root',
             'msr-safe-service',
             'msr-sync-root',
             'msr-sync-service',
             'msr-uring-root',
             'msr-uring-service']
    time_samples = dict()
    for cc in cases:
        time_samples[cc] = []
        with open(f'{cc}.csv') as fid:
            fid.readline()
            for line in fid.readlines():
                (count, time) = line.split(',')
                time_samples[cc].append(float(time))

    height = [sum(vv)/len(vv) for vv in time_samples.values()]
    plt.bar(x=range(len(time_samples)), height=height, tick_label=time_samples.keys())
if __name__ == '__main__':
    main()
