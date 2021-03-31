/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <mpi.h>

#include <vector>

#ifdef _OPENMP
#include <omp.h>
#else
static int omp_get_num_threads(void) {return 1;}
static int omp_get_thread_num(void) {return 0;}
#endif

#include "geopm.h"

int main(int argc, char **argv)
{
    int err = 0;
    int comm_rank = -1;
    int comm_size = -1;
    err = MPI_Init(&argc, &argv);
    if (!err) {
        err = MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    }

    int num_threads = 0;
# pragma omp parallel
{
    if (omp_get_thread_num() == 0) {
        num_threads = omp_get_num_threads();
    }
}

    int per_thread = 100;
    int repeat = num_threads * per_thread;
    std::vector<double> in_buffer(10000000, comm_rank);
    std::vector<double> out_buffer(10000000, 0.0);

#pragma omp parallel for
    for (int rep_idx = 0; rep_idx < repeat; ++rep_idx) {
        if (omp_get_thread_num() == 0) {
            if (!err) {
                MPI_Allreduce(in_buffer.data(), out_buffer.data(), in_buffer.size(),
                              MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            }
        }
    }
    MPI_Finalize();
    return err;
}
