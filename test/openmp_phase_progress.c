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
#include <limits.h>
#include <omp.h>

inline double do_something(int input)
{
    int i;
    double result = (double)input;
    for (i = 0; i < 1000; i++) {
        result += i*result;
    }
    return result;
}

void geopm_phase_progress_threaded(int num_thread, int *progress, double norm)
{
    static unsigned int num_calls = 0;
    int total_progress = INT_MAX;
    int j;
    double frac;

    for (j = 0; j < num_thread; ++j) {
        total_progress = progress[j] < total_progress ?
                         progress[j] : total_progress;
    }
    frac = total_progress * norm;

    /* DEBUG */
    num_calls++;
    if (num_calls%1024 == 0) {
        fprintf(stdout, "%f\n", frac);
    }
}

int main(int argc, char **argv)
{
    double x = 0;
    int *progress = (int *)calloc(omp_get_max_threads(), sizeof(int));
    int i, n = 1000000;
    int total_progress;
    double norm = omp_get_max_threads() / ((double)n);

    #pragma omp parallel for default(shared) private(i)
    for (i = 0; i < n; ++i) {
        x += do_something(i);
        progress[omp_get_thread_num()]++;
        if (omp_get_thread_num() == 0) {
            geopm_phase_progress_threaded(omp_get_num_threads(), progress, norm);
        }
    }
    free(progress);
}
