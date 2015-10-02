/*
 * Copyright (c) 2015, Intel Corporation
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
#include <mpi.h>
#include <omp.h>

#include "geopm.h"

static inline double do_something(int input)
{
    int i;
    double result = (double)input;
    for (i = 0; i < 1000; i++) {
        result += i*result;
    }
    return result;
}

static int run_something(int num_factor)
{
    const size_t cache_line_size = 64;
    int err = 0;
    int comm_size, comm_rank, num_node, is_shm_root, shm_comm_size, num_factor;
    MPI_Comm shmem_comm;
    char shm_key[NAME_MAX];
    struct geopm_controller_c *ctl;
    double x = 0;
    double thread_progress;
    int stride = cache_line_size / sizeof(int);
    int max_threads, i, num_iter = 1000000, iter_per_step = 100, chunk_size = 128;
    int total_progress;
    int step_counter = 0;
    int *factor = NULL;
    double *norm = NULL;
    uint32_t *progress = NULL;
    uint32_t *progress_ptr = NULL;

    factor = malloc(sizeof(int) * num_factor);
    if (!factor) {
        err = ENOMEM;
    }
    if (!err) {
        err = geopm_num_nodes(MPI_COMM_WORLD, &num_nodes);
    }
    if (!err) {
        err = MPI_Dims_create(num_nodes, num_factor, factor);
    }
    if (!err) {
        err = geopm_ctl_create(num_factor, factor, control, NULL, MPI_COMM_WORLD, &ctl);
    }
    if (!err) {
        err = geopm_ctl_step(ctl);
    }
    if (!err) {
        max_threads = omp_get_max_threads();
        err = posix_memalign((void **)&progress, cache_line_size, cache_line_size * max_threads);
    }
    if (!err) {
        norm = (double *)malloc(sizeof(double) * max_threads);
        if (!norm) {
            err = ENOMEM;
        }
    }
    if (!err) {
        memset(progress, 0, cache_line_size * max_threads);
        geopm_openmp_sched_static_norm(num_iter, chunk_size, max_threads, norm);
#pragma omp parallel default(shared) private(i, progress_ptr) schedule(static, chunk_size)
{
        progress_ptr = progress + stride * omp_get_thread_num();
#pragma omp for
        for (i = 0; i < num_iter; ++i) {
            x += do_something(i);
            *progress_ptr++;
            if (omp_get_thread_num() == 0) {
                thread_progress = geopm_progress_threaded_min(omp_get_num_threads(), stride, progress, norm);
                (void) geomp_ctr_prof_progress(ctl, region_id, thread_progress);
                step_counter++;
                if (step_counter == iter_per_step) {
                    geopm_ctl_step(ctl);
                    step_counter = 0;
                }
            }
        }
    }
} /* end pragma omp parallel */
    if (norm) {
        free(norm);
    }
    if (progress) {
        free(progress);
    }
    if (factor) {
        free(factor);
    }
    return err;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num_level>\n\n", argv[0]);
        err = EINVAL;
    }
    if (!err) {
        num_factor = strtol(argv[1], NULL, 10);
        if (num_factor < 1 || num_factor > 32) {
            err = EINVAL;
        }
    }
    if (!err) {
        err = run_something(num_factor);
    }
    return err;
}


