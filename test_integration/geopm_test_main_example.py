#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

import mpi4py.MPI
import sys
import geopmpy.bench
import geopmpy.prof

# Example test that runs a mixed region
G_REPEAT = 500
G_REGIONS = ['stream', 'dgemm', 'stream', 'all2all']
G_BIG_O = [0.01, 0.01, 0.01, 0.01]

def root_print(msg):
    rank = mpi4py.MPI.COMM_WORLD.Get_rank()
    if rank == 0:
        sys.stdout.write(msg)

def main():
    is_verbose = '--verbose' in sys.argv or '-v' in sys.argv
    model_region_list = []
    for (region_name, big_o) in zip(G_REGIONS, G_BIG_O):
        region_name += '-unmarked'
        model_region_list.append(geopmpy.bench.model_region_factory(region_name, big_o, is_verbose))

    root_print('Beginning loop of {} iterations.\n'.format(G_REPEAT))

    region_id = geopmpy.prof.region('mixed', geopmpy.prof.REGION_HINT_UNKNOWN)
    for iter in range(G_REPEAT):
        geopmpy.prof.epoch()
        geopmpy.prof.region_enter(region_id)
        for model_region in model_region_list:
            geopmpy.bench.model_region_run(model_region)
        geopmpy.prof.region_exit(region_id)
        root_print('Iteration: {}\r'.format(iter))

if __name__ == '__main__':
    main()
