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
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "geopm.h"
#include "tutorial_region.h"
#ifdef TUTORIAL_ENABLE_MKL
#include "mkl.h"
#endif

int tutorial_sleep(double big_o, int do_report)
{
    int err = 0;
    if (big_o != 0.0) {
        struct timespec seconds = {(time_t)(big_o),
                                   (long)((big_o -
                                   (time_t)(big_o)) * 1E9)};
        if (do_report) {
            printf("Sleeping for %e seconds\n", big_o);
            fflush(stdout);
        }

        err = clock_nanosleep(CLOCK_REALTIME, 0, &seconds, NULL);
    }
    return err;
}

int tutorial_dgemm(double big_o, int do_report)
{
    int err = 0;
    if (big_o != 0.0) {
        int matrix_size = (int) pow(4e9 * big_o, 1.0/3.0);
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

        err = posix_memalign((void *)&A, pad_size, mem_size);
        if (!err) {
            err = posix_memalign((void *)&B, pad_size, mem_size);
        }
        if (!err) {
            err = posix_memalign((void *)&C, pad_size, mem_size);
        }

        if (!err) {
#pragma omp parallel for
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
            free(C);
            free(B);
            free(A);
        }
    }
    return err;
}

int tutorial_stream(double big_o, int do_report)
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
                printf("Executing STREAM triad on length %d vectors.\n", num_stream);
                fflush(stdout);
            }

#pragma omp parallel for
            for (int i = 0; i < num_stream; ++i) {
                a[i] = b[i] + scalar * c[i];
            }
            free(c);
            free(b);
            free(a);
        }
    }
    return err;
}

int tutorial_all2all(double big_o, int do_report)
{
    /* Best case scaling is O(ln(num_send) + num_rank) => */
    /*     num_send = exp(big_o_n - factor * num_rank) */
    /* We have somewhat arbitrarily set factor to 1/128 */
    int err = 0;
    if (big_o != 0.0) {
        int num_rank = 0;
        int err = MPI_Comm_size(MPI_COMM_WORLD, &num_rank);
        size_t num_send = (size_t)pow(2.0, 16 * big_o - num_rank / 128.0);
        num_send = num_send ? num_send : 1;
        size_t cline_size = 64;
        char *send_buffer = NULL;
        char *recv_buffer = NULL;
        if (!err) {
            err = posix_memalign((void *)&send_buffer, cline_size,
                                 num_rank * num_send * sizeof(char));
        }
        if (!err) {
            err = posix_memalign((void *)&recv_buffer, cline_size,
                                 num_rank * num_send * sizeof(char));
        }
        if (!err) {
            if (do_report) {
                printf("Executing all2all of %d byte buffer on %d ranks.\n",
                       num_send * sizeof(char), num_rank);
                fflush(stdout);
            }
            err = MPI_Alltoall(send_buffer, num_send, MPI_CHAR, recv_buffer,
                               num_send, MPI_CHAR, MPI_COMM_WORLD);
        }

        if (!err) {
            err = MPI_Barrier(MPI_COMM_WORLD);
        }
        if (!err) {
            free(recv_buffer);
            free(send_buffer);
        }
    }
    return err;
}

int tutorial_dgemm_static(double big_o, int do_report)
{
    static double big_o_last = 0.0;
    static double *A = NULL;
    static double *B = NULL;
    static double *C = NULL;

    int err = 0;
    if (big_o != 0.0) {
        int matrix_size = (int) pow(4e9 * big_o, 1.0/3.0);
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

        if (big_o != big_o_last) {
            big_o_last = big_o;
            if (A) {
                free(C);
                free(B);
                free(A);
                A = NULL;
                B = NULL;
                C = NULL;
            }
            err = posix_memalign((void *)&A, pad_size, mem_size);
            if (!err) {
                err = posix_memalign((void *)&B, pad_size, mem_size);
            }
            if (!err) {
                err = posix_memalign((void *)&C, pad_size, mem_size);
            }

            if (!err) {
#pragma omp parallel for
                for (int i = 0; i < mem_size / sizeof(double); ++i) {
                    A[i] = random() / RAND_MAX;
                    B[i] = random() / RAND_MAX;
                }
            }
        }
        if (!err) {
            if (do_report) {
                printf("Executing a %d x %d DGEMM\n", matrix_size, matrix_size);
                fflush(stdout);
            }

            dgemm(&transa, &transb, &M, &N, &K, &alpha,
                  A, &LDA, B, &LDB, &beta, C, &LDC);
        }
    }
    else if (A) {
        free(C);
        free(B);
        free(A);
        A = NULL;
        B = NULL;
        C = NULL;
    }
    return err;
}

#ifdef _OPENMP
static int stream_profiled_omp(uint64_t region_id, size_t num_stream, double scalar, double *a, double *b, double *c)
{
    const size_t block = 256;
    const size_t num_block = num_stream / block;
    const size_t num_remain = num_stream % block;
    int err = 0;
    int num_thread = 1;

#pragma omp parallel
{
    num_thread = omp_get_num_threads();
}
#pragma omp parallel
{
    int thread_idx = omp_get_thread_num();
    (void)geopm_tprof_init_loop(num_thread, thread_idx, num_block, 0);

#pragma omp for
    for (size_t i = 0; i < num_block; ++i) {
        for (size_t j = 0; j < block; ++j) {
            a[i * block + j] = b[i * block + j] + scalar * c[i * block + j];
        }
        (void)geopm_tprof_post();
    }
#pragma omp for
        for (size_t j = 0; j < num_remain; ++j) {
            a[num_block * block + j] = b[num_block * block + j] + scalar * c[num_block * block + j];
        }
}
    }

    return err;
}
#endif

static int stream_profiled_serial(uint64_t region_id, size_t num_stream, double scalar, double *a, double *b, double *c)
{
    const size_t block = 256;
    const size_t num_block = num_stream / block;
    const size_t num_remain = num_stream % block;
    const double norm = 1.0 / num_block;

    for (size_t i = 0; i < num_block; ++i) {
        for (size_t j = 0; j < block; ++j) {
            a[i * block + j] = b[i * block + j] + scalar * c[i * block + j];
        }
        geopm_prof_progress(region_id, i * norm);
    }
    for (size_t j = 0; j < num_remain; ++j) {
        a[num_block * block + j] = b[num_block * block + j] + scalar * c[num_block * block + j];
    }

    return 0;
}

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
                printf("Executing profiled STREAM triad on length %d vectors.\n", num_stream);
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
}
