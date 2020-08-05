#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

'''
Shows balancer chosen power limits on each socket over time.

'''

import pandas
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
import sys
import os


def plot_lines(traces):
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
        # TODO: try catch and warn about using the balancer
        tgt = df['POLICY_MAX_EPOCH_RUNTIME']

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
    # draw target once on top of other lines
    axs[1].plot(time, tgt, label='target')
    fig.legend(loc='lower right')
    fig.savefig(os.path.join(os.path.dirname(traces[0]), 'balancer_power_limits.png'))


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stderr.write('Provide paths to trace file(s)')
        sys.exit(1)
    plot_lines(sys.argv[1:])
