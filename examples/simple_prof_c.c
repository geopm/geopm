/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <mpi.h>
#include <omp.h>

#include "geopm.h"


int main(int argc, char **argv)
{
    int chunk_size = 0;
    int err = 0;
    int index = 0;
    int rank = 0;
    int num_iter = 100000000;
    double sum = 0.0;
    int num_thread = 0;
    uint64_t region_id = 0;

    err = MPI_Init(&argc, &argv);
    if (!err) {
#pragma omp parallel
{
        num_thread = omp_get_num_threads();
}
        chunk_size = num_iter / num_thread;
        if (num_iter % num_thread) {
            ++chunk_size;
        }
    }
    if (!err) {
        err = geopm_prof_region("loop_0", GEOPM_REGION_HINT_UNKNOWN, &region_id);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!err) {
        err = geopm_prof_enter(region_id);
    }
    if (!err) {
#pragma omp parallel default(shared) private(index)
{
        uint32_t thread_idx = omp_get_thread_num();
        (void)geopm_tprof_reset_loop(num_thread, thread_idx, num_iter, chunk_size);
#pragma omp for reduction(+:sum) schedule(static, chunk_size)
        for (index = 0; index < num_iter; ++index) {
            sum += (double)index;
            (void)geopm_tprof_increment();
        }
}
        err = geopm_prof_exit(region_id);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    if (!err && !rank) {
        printf("sum = %e\n\n", sum);
    }

    int tmp_err = MPI_Finalize();

    return err ? err : tmp_err;
}
