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
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "tutorial_region.h"

void dgemm(const char *transa, const char *transb, const int *M,
           const int *N, const int *K, const double *alpha,
           const double *A, const int *LDA, const double *B,
           const int *LDB, const double *beta, double *C, const int *LDC);


#ifndef TUTORIAL_ENABLE_BLAS
void dgemm(const char *transa, const char *transb, const int *M,
           const int *N, const int *K, const double *alpha,
           const double *A, const int *LDA, const double *B,
           const int *LDB, const double *beta, double *C, const int *LDC)
{
    // Terrible DGEMM implementation if there is no BLAS
#pragma omp parallel for
    for (int i = 0; i < *M; ++i) {
        for (int j = 0; j < *N; ++j) {
            C[i * *LDC + j] = 0;
            for (int k = 0; k < *K; ++k) {
                C[i * *LDC + j] += A[i * *LDA + j] * B[j * *LDB + k];
            }
        }
    }
}
#endif


int tutorial_sleep(double big_o, int do_report)
{
    struct timespec seconds = {(time_t)(big_o),
                               (long)((big_o -
                               (time_t)(big_o)) * 1E9)};
    if (do_report) {
        printf("Sleeping for %e seconds\n", big_o);
        fflush(stdout);
    }

    return clock_nanosleep(CLOCK_REALTIME, 0, &seconds, NULL);
}

int tutorial_dgemm(double big_o, int do_report)
{
    int matrix_size = (int) pow(big_o, 1.0/3.0);
    int pad_size = 64;
    size_t mem_size = sizeof(double) * (matrix_size * (matrix_size + pad_size));
    char transa = 'n';
    char transb = 'n';
    int M = matrix_size;
    int N = matrix_size;
    int K = matrix_size;
    int LDA = matrix_size + pad_size / sizeof(double);
    int LDB = matrix_size + pad_size / sizeof(double);
    int LDC = matrix_size + pad_size / sizeof(double);
    double alpha = 2.0;
    double beta = 3.0;
    double *A = NULL;
    double *B = NULL;
    double *C = NULL;

    int err = posix_memalign((void *)&A, pad_size, mem_size);
    if (!err) {
        err = posix_memalign((void *)&B, pad_size, mem_size);
    }
    if (!err) {
        err = posix_memalign((void *)&C, pad_size, mem_size);
    }

    if (!err) {
        for (int i = 0; i < mem_size / sizeof(double); ++i) {
            A[i] = random() / RAND_MAX;
            B[i] = random() / RAND_MAX;
        }

        if (do_report) {
            printf("Executing a %d x %d DGEMM\n", matrix_size, matrix_size);
            fflush(stdout);
        }

        dgemm(&transa, &transb, &M, &N, &K, &alpha,
              A, &LDA, B, &LDB, &beta, C, &LDC);
    }
    return err;
}


int tutorial_stream(double big_o_n, int do_report)
{
    size_t cline_size = 64;
    size_t num_stream = (size_t)big_o_n;
    size_t mem_size = sizeof(double) * num_stream;
    double *a = NULL;
    double *b = NULL;
    double *c = NULL;
    double scalar = 3.0;

    int err = posix_memalign((void *)&a, cline_size, mem_size);
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
            printf("Executing STREAM triad on length %d vectors.\n", num_stream);
            fflush(stdout);
        }

#pragma omp parallel for
        for (int i = 0; i < num_stream; ++i) {
            a[i] = b[i] + scalar * c[i];
        }
    }
    return err;
}

int tutorial_all2all(double big_o_n, int do_report)
{
    /* Best case scaling is O(ln(num_send) + num_rank) => */
    /*     num_send = exp(big_o_n - factor * num_rank) */
    /* We have somewhat arbitrarily set factor to 1/128 */
    int num_rank = 0;
    int err = MPI_Comm_size(MPI_COMM_WORLD, &num_rank);
    size_t num_send = (size_t)pow(2.0, big_o_n - num_rank / 128.0);
    num_send = num_send ? num_send : 1;
    size_t cline_size = 64;
    char *send_buffer = NULL;
    char *recv_buffer = NULL;
    if (!err) {
        err = posix_memalign((void *)&send_buffer, cline_size, num_rank * num_send * sizeof(char));
    }
    if (!err) {
        err = posix_memalign((void *)&recv_buffer, cline_size, num_rank * num_send * sizeof(char));
    }
    if (!err) {
        if (do_report) {
            printf("Executing all2all of %d byte buffer on %d ranks.\n", num_send * sizeof(char), num_rank);
            fflush(stdout);
        }
        err = MPI_Alltoall(send_buffer, num_send, MPI_CHAR, recv_buffer, num_send, MPI_CHAR, MPI_COMM_WORLD);
    }

    if (!err) {
        err = MPI_Barrier(MPI_COMM_WORLD);
    }
    return err;
}
