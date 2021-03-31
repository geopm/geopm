#!/usr/bin/env python
#
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

'''
Displays a historgram of the control loop sample intervals
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
