#!/usr/bin/env python3
#  Copyright (c) 2015 - 2021, Intel Corporation
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
