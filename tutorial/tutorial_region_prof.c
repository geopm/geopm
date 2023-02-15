/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <geopm_prof.h>
#include <geopm_hint.h>

#include "tutorial_region.h"

#ifdef _OPENMP
static int stream_profiled_omp(uint64_t region_id, size_t num_stream, double scalar, double *a, double *b, double *c)
{
    const size_t block = 256;
    const size_t num_block = num_stream / block;
    const size_t num_remain = num_stream % block;
    int err = 0;

#pragma omp parallel for
    for (size_t i = 0; i < num_block; ++i) {
        for (size_t j = 0; j < block; ++j) {
            a[i * block + j] = b[i * block + j] + scalar * c[i * block + j];
        }
        geopm_tprof_post();
    }
#pragma omp parallel for
    for (size_t j = 0; j < num_remain; ++j) {
        a[num_block * block + j] = b[num_block * block + j] + scalar * c[num_block * block + j];
    }

    return err;
}
#else
static int stream_profiled_serial(uint64_t region_id, size_t num_stream, double scalar, double *a, double *b, double *c)
{
    const size_t block = 256;
    const size_t num_block = num_stream / block;
    const size_t num_remain = num_stream % block;

    geopm_tprof_init(num_block);
    for (size_t i = 0; i < num_block; ++i) {
        for (size_t j = 0; j < block; ++j) {
            a[i * block + j] = b[i * block + j] + scalar * c[i * block + j];
        }
        geopm_tprof_post();
    }
    for (size_t j = 0; j < num_remain; ++j) {
        a[num_block * block + j] = b[num_block * block + j] + scalar * c[num_block * block + j];
    }

    return 0;
}
#endif

int tutorial_stream_profiled(double big_o, int do_report)
{
    int err = 0;
    if (big_o != 0.0) {
        size_t cline_size = 64;
        size_t num_stream = (size_t)big_o * 500000000;
        size_t mem_size = sizeof(double) * num_stream;
        double *a = NULL;
        double *b = NULL;
        double *c = NULL;
        double scalar = 3.0;
        uint64_t stream_rid;

        if (!err) {
            err = geopm_prof_region("tutorial_stream",
                                    GEOPM_REGION_HINT_MEMORY,
                                    &stream_rid);
        }

        err = posix_memalign((void *)&a, cline_size, mem_size);
        if (!err) {
            err = posix_memalign((void *)&b, cline_size, mem_size);
        }
        if (!err) {
            err = posix_memalign((void *)&c, cline_size, mem_size);
        }
        if (!err) {
#pragma omp parallel for
            for (int i = 0; i < num_stream; i++) {
                a[i] = 0.0;
                b[i] = 1.0;
                c[i] = 2.0;
            }

            if (do_report) {
                printf("Executing profiled STREAM triad on length %ld vectors.\n", num_stream);
                fflush(stdout);
            }
            err = geopm_prof_enter(stream_rid);
        }
        if (!err) {
#ifdef _OPENMP
            err = stream_profiled_omp(stream_rid, num_stream, scalar, a, b, c);
#else
            err = stream_profiled_serial(stream_rid, num_stream, scalar, a, b, c);
#endif
        }
        if (!err) {
            err = geopm_prof_exit(stream_rid);
        }
        if (!err) {
            free(c);
            free(b);
            free(a);
        }
    }
    return err;
}
