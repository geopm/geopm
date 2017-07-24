#!/usr/bin/env python
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

import matplotlib.pyplot as plt
import sys
import glob

def plot_data(file_name_prefix):
    file_name_match = glob.glob(file_name_prefix + '*')
    for file_name in file_name_match:
        limit_key = 'Power limit (Watts): '
        time_key = 'Total time (seconds): '
        meas_key = 'Average power (Watts): '
        limit_list = []
        time_list = []
        meas_list = []
        with open(file_name) as fid:
            for line in fid.readlines():
                if line.startswith(limit_key):
                    limit_list.append(float(line.split(':')[1]))
                elif line.startswith(time_key):
                    time_list.append(float(line.split(':')[1]))
                elif line.startswith(meas_key):
                    meas_list.append(float(line.split(':')[1]))
            if (len(limit_list) != len(time_list) or
                len(limit_list) != len(meas_list)):
                raise RuntimeError('Failed to parse file {}'.format(file_name))

            limit_uniq = sorted(list(set(limit_list)))

            base_name = file_name.split('.')[0]
            out_name = base_name + '.png'
            plt.plot(limit_uniq, limit_uniq, '-')
            plt.plot(limit_list, meas_list, '.')
            if base_name.startswith('rapl_pkg_limit_test_'):
                base_name = base_name[len('rapl_pkg_limit_test_'):]
            plt.title(base_name)
            plt.xlabel('RAPL setting (Watts)')
            plt.ylabel('RAPL power measured (Watts)')
            plt.savefig(out_name)
            plt.close()

if __name__ == '__main__':
    plot_data(sys.argv[1])
