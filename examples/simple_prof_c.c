/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
    int ierr = 0;
    int index = 0;
    int rank = 0;
    int num_iter = 100000000;
    double sum = 0.0;
    struct geopm_prof_c *prof = NULL;
    int num_thread = 0;
    int thread_idx = 0 ;
    int cache_line_size = 64;
    int stride = cache_line_size / sizeof(int32_t);
    uint64_t region_id = 0;
    uint32_t *progress = NULL;
    double *norm = NULL;
    double min_progress = 0.0;

    ierr = MPI_Init(&argc, &argv);
    if (!ierr) {
#pragma omp parallel
        {
            num_thread = omp_get_num_threads();
        }
        ierr = posix_memalign((void **)&progress, cache_line_size, cache_line_size * num_thread * sizeof(int32_t));
    }
    if (!ierr) {
        memset(progress, 0, cache_line_size * num_thread * sizeof(int32_t));
        norm = malloc(num_thread * sizeof(double));
        if (!norm) {
            ierr = ENOMEM;
        }
    }
    if (!ierr) {
        chunk_size = num_iter / num_thread;
        ierr = geopm_omp_sched_static_norm(num_iter, chunk_size, num_thread, norm);
    }
    if (!ierr) {
        ierr = geopm_prof_create("timed_loop", NULL, MPI_COMM_WORLD, &prof);
    }
    if (!ierr) {
        ierr = geopm_prof_region(NULL, "loop_0", GEOPM_POLICY_HINT_UNKNOWN, &region_id);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!ierr) {
        ierr = geopm_prof_enter(NULL, region_id);
    }
    if (!ierr) {
#pragma omp parallel default(shared) private(thread_idx, index)
{
        thread_idx = omp_get_thread_num();
#pragma omp for reduction(+:sum) schedule(static, chunk_size)
        for (index = 0; index < num_iter; ++index) {
            sum += (double)index;
            progress[stride * thread_idx]++;
            if (!thread_idx) {
                min_progress = geopm_progress_threaded_min(num_thread, stride, progress, norm);
                (void) geopm_prof_progress(NULL, region_id, min_progress);
            }
        }
}
        ierr = geopm_prof_exit(NULL, region_id);
    }
    if (!ierr) {
        ierr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    if (!ierr && !rank) {
        printf("sum = %e\n\n", sum);
    }
    if (!ierr) {
        ierr = geopm_prof_print(prof, "timed_loop", 0);
    }
    if (!ierr) {
        ierr = geopm_prof_destroy(prof);
    }

    int tmp_err = MPI_Finalize();

    return ierr ? ierr : tmp_err;
}
