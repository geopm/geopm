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
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <iomanip>

#include "geopm_time.h"

/// function declarations
extern "C" {
int dgemm_(char *transa, char *transb, int *m, int *n, int *k,
           double *alpha, double *a, int *lda, double *b, int *ldb,
           double *beta, double *c, int *ldc);
}

/// macro definitions
#define MAX_SAMPLES             1000
#define NUM_TESTS               1000 // TODO command line configurable

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 10240ULL
#endif
#ifndef PAD_SIZE
#define PAD_SIZE 128ULL
#endif
#ifndef NUM_DGEMM_REP
#define NUM_DGEMM_REP 400
#endif
#ifndef NAME_MAX
#define NAME_MAX 512
#endif

#define MAX_NUM_SOCKET 16

#define COL_WIDTH               16
#define CONFIG_COL              std::setw(COL_WIDTH)

int main(int argc, char **argv)
{
    const struct geopm_time_s true_zero = {{0, 0}};
    struct geopm_time_s time_zero = {{0, 0}}, start_time = {{0, 0}}, end_time = {{0, 0}};
    double *aa = NULL;
    double *bb = NULL;
    double *cc = NULL;

    /* Allocate some memory */
    const size_t num_elements = MATRIX_SIZE * (MATRIX_SIZE + PAD_SIZE);

    int err = posix_memalign((void **)&aa, 64, num_elements * sizeof(double));
    if (!err) {
        err = posix_memalign((void **)&bb, 64, num_elements * sizeof(double));
    }
    if (!err) {
        err = posix_memalign((void **)&cc, 64, num_elements * sizeof(double));
    }
    if (!err) {
        /* setup matrix values */
        memset(cc, 0, num_elements * sizeof(double));
        for (int i = 0; i < num_elements; ++i) {
            aa[i] = 1.0;
            bb[i] = 2.0;
        }
    }

    /* Run dgemm in a loop */
    int M = MATRIX_SIZE;
    int N = MATRIX_SIZE;
    int K = MATRIX_SIZE;
    int P = PAD_SIZE;
    int LDA = MATRIX_SIZE + PAD_SIZE;
    int LDB = MATRIX_SIZE + PAD_SIZE;
    int LDC = MATRIX_SIZE + PAD_SIZE;
    double alpha = 2.0;
    double beta = 3.0;
    char transa = 'n';
    char transb = 'n';
    geopm_time(&time_zero);
    std::cout << geopm_time_diff(&true_zero, &time_zero) << std::endl << std::endl;
    std::cout.fill(' ');
    std::cout << CONFIG_COL << "start" << CONFIG_COL << "end" << std::endl;
    for (int i = 0; i < NUM_DGEMM_REP; ++i) {
        geopm_time(&start_time);
        dgemm_(&transa, &transb, &M, &N, &K, &alpha,
                aa, &LDA, bb, &LDB, &beta, cc, &LDC);
        geopm_time(&end_time);
        std::cout << CONFIG_COL << geopm_time_diff(&time_zero, &start_time) << CONFIG_COL << geopm_time_diff(&time_zero, &end_time) << std::endl;
    }

    if (aa) free(aa);
    if (bb) free(bb);
    if (cc) free(cc);

    return 0;
}
