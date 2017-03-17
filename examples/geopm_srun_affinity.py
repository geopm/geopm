#!/usr/bin/env python
#
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
"""
    geopm_srun_affinity.py <mode> <num_rank> <num_thread>
        mode: One of the following strings
              "process" - geopm controller as an mpi process.
              "thread" - geopm controller as a posix thread.
              "geopmctl" - for geopmctl application.
              "application" - for main application when launching with geopmctl.
        num_rank: Number of ranks per node used by main application.
        num_thread: Number of threads per rank used by main application.
"""
import sys

def geopm_srun_affinity(mode, num_rank, num_thread):
    """
    geopm_srun_affinty(mode, num_rank, num_thread)
        mode: One of the following strings
              "process" - geopm controller as an mpi process.
              "thread" - geopm controller as a posix thread.
              "geopmctl" - for geopmctl application.
              "application" - for main application when launching with geopmctl.
        num_rank: Number of ranks per node used by main application.
        num_thread: Number of threads per rank used by main application.
    """
    result_base = '--cpu_bind=v,mask_cpu:'
    if mode == 'geopmctl':
        result = result_base + '0x1'
    else:
        mask_list = []
        if mode == 'process':
            mask_list.append('0x1')
            binary_mask = num_thread * '1' + '0'
        elif mode == 'thread':
            binary_mask = (num_thread + 1) * '1'
        elif mode == 'application':
            binary_mask = num_thread * '1' + '0'
        else:
            raise NameError('Unknown mode: "{mode}", valid options are "process", "thread", "geopmctl", or "application"'.format(mode=mode))
        for ii in range(num_rank):
            hex_mask = '0x{:x}'.format(int(binary_mask, 2))
            mask_list.append(hex_mask)
            if ii == 0 and mode == 'thread':
                binary_mask = num_thread * '1' + '0'
            binary_mask = binary_mask + num_thread * '0'

        result = result_base + ','.join(mask_list)
    return result

if __name__ == '__main__':
    try:
        mode = sys.argv[1]
        num_rank = int(sys.argv[2])
        num_thread = int(sys.argv[3])
    except Exception as e:
        print __doc__
        sys.exit(-1)
    try:
        sys.stdout.write(geopm_srun_affinity(mode, num_rank, num_thread) + '\n')
    except Exception as e:
        print e
        sys.exit(-1)

