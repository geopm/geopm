#!/usr/bin/env python3
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from time import time
from time import sleep
from geopmdpy import pio
from geopmdpy import topo


def get_all_msrs():
    return [nn for nn in pio.signal_names()
            if nn.startswith('MSR::') and
            pio.signal_domain_type(nn) == topo.domain_type('core')]

def get_signals(signal_idx):
    result = []
    for idx in signal_idx:
        result.append(pio.sample(idx))
    return result


def push_msrs(all_msrs):
    signal_idx = []
    for cc in range(topo.num_domain('core')):
        for msr in all_msrs:
            idx = pio.push_signal('SERVICE::' + msr, 'core', cc)
            signal_idx.append(idx)
    return signal_idx

def main():
    ta = time()
    all_msrs = get_all_msrs()
    tb = time()
    print(f'pio.signal_names(): {tb - ta} sec')
    ta = time()
    signal_idx = push_msrs(all_msrs)
    tb = time()
    print(f'pio.push_signal(): {tb - ta} sec')
    for trial in range(3):
        print(f'Trial {trial}')
        ta = time()
        pio.read_batch()
        tb = time()
        print(f'pio.read_batch(): {tb - ta} sec')
        ta = time()
        signal = get_signals(signal_idx)
        tb = time()
        print(f'pio.sample(): {tb - ta} sec')
        sleep(0.05)

if __name__ == '__main__':
    main()
