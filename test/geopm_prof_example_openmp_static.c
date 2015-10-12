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
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <omp.h>
#include <stdint.h>
#include <errno.h>

#ifndef _OPENMP
#error "Requires OpenMP"
#endif

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

#ifndef MOCK_GEOPM
#include "geopm.h"
#else
/* The following mock of the geopm_prof_c interface can be used to
   compile this example without linking to libgeopm.  */

enum geopm_sample_reduce_e {
    GEOPM_SAMPLE_REDUCE_THREAD = 1,
    GEOPM_SAMPLE_REDUCE_PROC = 2,
    GEOPM_SAMPLE_REDUCE_NODE = 3,
};

struct geopm_prof_c {
    char m_name[NAME_MAX];
};

int geopm_prof_progress(struct geopm_prof_c *prof, int region_id, double fraction)
{
    printf("geopm_prof_progress(prof->name=%s, region_id=%d, fraction: %f\n", prof->m_name, region_id, fraction);
    return 0;
}

double geopm_progress_threaded_min(int num_thread,
                                   size_t stride,
                                   const uint32_t *progress,
                                   const double *norm)
{
    double progress_min = DBL_MAX;
    double progress_tmp;
    int j;

    for (j = 0; j < num_thread; ++j) {
        progress_tmp = progress[j * stride] * norm[j];
        progress_min =  progress_tmp < progress_min ?
                        progress_tmp : progress_min;
    }
    return progress_min;
}

int geopm_omp_sched_static_norm(int num_iter, int chunk_size, int num_thread, double *norm)
{
    int remain = num_iter;
    int i = 0;

    /* inefficient but robust way of calculating the norm based on
       OpenMP documentation. */
    memset(norm, 0, sizeof(double) * num_thread);
    while (remain) {
        if (remain > chunk_size) {
            norm[i] += chunk_size;
            remain -= chunk_size;
        }
        else {
            norm[i] += remain;
            remain = 0;
        }
        i++;
        if (i == num_thread) {
            i = 0;
        }
    }
    for (i = 0; i < num_thread; ++i) {
        norm[i] = 1.0 / norm[i];
    }
    return 0;
}

int geopm_prof_create(const char *name, int sample_reduce, const char *sample_key, struct geopm_prof_c **prof)
{
    int err = 0;
    *prof = calloc(1, sizeof(struct geopm_prof_c));
    if (*prof == NULL) {
        err = ENOMEM;
    }
    if (!err) {
        strncpy((*prof)->m_name, name, NAME_MAX);
    }
    if ((*prof)->m_name[NAME_MAX-1] != '\0') {
        err = -1;
    }
    return err;
}

int geopm_prof_destroy(struct geopm_prof_c *prof)
{
    free(prof);
    return 0;
}

int geopm_prof_register(struct geopm_prof_c *prof, const char *region_name, long policy_hint, int *region_id)
{
    *region_id = 1;
    return 0;
}

int geopm_prof_enter(struct geopm_prof_c *prof, int region_id)
{
    return 0;
}

int geopm_prof_exit(struct geopm_prof_c *prof, int region_id)
{
    return 0;
}

#endif /* end mock of geopm_prof interface */

struct geopm_prof_c *prof_g;

inline void error_handler(int err)
{
    if (err) {
        fprintf(stderr, "Error: %i, fatal\n\n", err);
        exit(err);
    }
}

inline double do_something(int input)
{
    int i;
    double result = (double)input;
    for (i = 0; i < 1000; i++) {
        result += i*result;
    }
    return result;
}


int main(int argc, char **argv)
{
    double x = 0;
    int stride = 64 / sizeof(int);
    int *progress = (int *)calloc(omp_get_max_threads() * stride, sizeof(int));
    int i;
    int region_id;
    int num_iter = 10000;
    int chunk_size = 1001;
    int num_thread = omp_get_max_threads();
    double *norm = (double *)calloc(num_thread, sizeof(double));

    error_handler(geopm_omp_sched_static_norm(num_iter, chunk_size, num_thread, norm));
    error_handler(geopm_prof_create("geopm_prof_c example", GEOPM_SAMPLE_REDUCE_PROC, NULL, &prof_g));
    error_handler(geopm_prof_register(prof_g, "main loop", 0, &region_id));
    error_handler(geopm_prof_enter(prof_g, region_id));
    #pragma omp parallel for default(shared) private(i) num_threads(num_thread) schedule(static, chunk_size)
    for (i = 0; i < num_iter; ++i) {
        x += do_something(i);
        progress[omp_get_thread_num() * stride]++;
        if (omp_get_thread_num() == 0) {
            error_handler(geopm_prof_progress(prof_g, region_id,
                geopm_progress_threaded_min(num_thread, stride, progress, norm)));
        }
    }
    error_handler(geopm_prof_exit(prof_g, region_id));
    error_handler(geopm_prof_destroy(prof_g));
    free(norm);
    free(progress);
}
