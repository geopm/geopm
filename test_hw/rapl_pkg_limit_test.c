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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <omp.h>

#include "geopm_time.h"

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 10240ULL
#endif
#ifndef PAD_SIZE
#define PAD_SIZE 128ULL
#endif
#ifndef NUM_REP
#define NUM_REP 10
#endif
#ifndef NAME_MAX
#define NAME_MAX 512
#endif

int dgemm_(char *transa, char *transb, int *m, int *n, int *k,
           double *alpha, double *a, int *lda, double *b, int *ldb,
           double *beta, double *c, int *ldc);

int main(int argc, char **argv)
{
    const char *usage = "%s package_limit_watts [num_rep]\n    default num_rep=10\n";
    const int matrix_size = MATRIX_SIZE;
    const int pad_size = PAD_SIZE;
    const size_t num_elements = matrix_size * (matrix_size + pad_size);
    const off_t pkg_power_unit_off = 0x606;
    const off_t pkg_power_limit_off = 0x610;
    const off_t pkg_energy_status_off = 0x611;
    int num_rep = NUM_REP;
    int err = 0;
    int fd = 0;
    double power_units = 0.0;
    double energy_units = 0.0;
    double power_limit = 0.0;
    double power_used = 0.0;
    double total_time = 0.0;
    uint64_t msr_value = 0;
    uint64_t save_limit = 0;
    uint64_t begin_energy = 0;
    uint64_t end_energy = 0;
    double *aa = NULL;
    double *bb = NULL;
    double *cc = NULL;
    struct geopm_time_s begin_time = {{0,0}};
    struct geopm_time_s end_time = {{0,0}};
    char hostname[NAME_MAX] = {0};
    char outfile_name[NAME_MAX] = {0};
    FILE *outfile = NULL;

    /* Read limit in watts from command line */
    if (argc < 2) {
        fprintf(stderr, usage, argv[0]);
        err = -1;
    }
    if (!err) {
        power_limit = atof(argv[1]);
        if (argc == 3) {
            num_rep = atoi(argv[2]);
        }
    }
    if (!err) {
        err = gethostname(hostname, NAME_MAX);
        if (err) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        int outlen = snprintf(outfile_name, NAME_MAX, "rapl_pkg_limit_test_%s.out", hostname);
        if (outlen >= NAME_MAX) {
            err = ENAMETOOLONG;
        }
    }
    if (!err) {
        outfile = fopen(outfile_name, "a");
        if (!outfile) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        fprintf(outfile, "###############################################################################\n");
        fprintf(outfile, "Power limit (Watts): %f\n", power_limit);
        fprintf(outfile, "Matrix size: %d\n", matrix_size);
        fprintf(outfile, "Pad size: %d\n", pad_size);
        fprintf(outfile, "Repetitions: %d\n", num_rep);
    }
    /* Allocate some memory */
    if (!err) {
        err = posix_memalign((void **)&aa, 64, num_elements * sizeof(double));
    }
    if (!err) {
        err = posix_memalign((void **)&bb, 64, num_elements * sizeof(double));
    }
    if (!err) {
        err = posix_memalign((void **)&cc, 64, num_elements * sizeof(double));
    }
    if (!err) {
        memset(cc, 0, num_elements * sizeof(double));
    }
    if (!err) {
        /* setup matrix values */
        for (int i = 0; i < num_elements; ++i) {
            aa[i] = 1.0;
            bb[i] = 2.0;
        }

        /* Read RAPL_PKG_POWER_LIMIT MSR */
        fd = open("/dev/cpu/0/msr_safe", O_RDWR);
        if (fd == -1) {
            errno = 0;
            fd = open("/dev/cpu/0/msr", O_RDWR);
        }
        if (fd == -1) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        /* Read RAPL_POWER_UNIT */
        errno = 0;
        pread(fd, &msr_value, sizeof(uint64_t), pkg_power_unit_off);
        err = errno;
    }
    if (!err) {
        power_units = pow(2.0, -1.0 * (msr_value & 0xF));
        energy_units = pow(2.0, -1.0 * ((msr_value >> 8) & 0x1F));
        /* Read RAPL_PKG_POWER_LIMIT */
        errno = 0;
        pread(fd, &msr_value, sizeof(uint64_t), pkg_power_limit_off);
        err = errno;
    }
    if (!err) {
        save_limit = msr_value;
        msr_value &= 0xFFFFFFFFFFFF0000;
        msr_value |= 0x000000000000FFFF & (uint64_t)(power_limit / power_units);
        /* Write RAPL_PKG_POWER_LIMIT */
        errno = 0;
        pwrite(fd, &msr_value, sizeof(uint64_t), pkg_power_limit_off);
        err = errno;
    }
    if (!err) {
        /* Get time of day */
        err = geopm_time(&begin_time);
    }
    if (!err) {
        /* Read package energy */
        errno = 0;
        pread(fd, &msr_value, sizeof(uint64_t), pkg_energy_status_off);
        err = errno;
    }
    if (!err) {
        begin_energy = msr_value & 0x00000000FFFFFFFF;

        int M = matrix_size;
        int N = matrix_size;
        int K = matrix_size;
        int P = pad_size;
        int LDA = matrix_size + pad_size;
        int LDB = matrix_size + pad_size;
        int LDC = matrix_size + pad_size;
        double alpha = 2.0;
        double beta = 3.0;
        char transa = 'n';
        char transb = 'n';
        for (int i = 0; i < num_rep; ++i) {
            dgemm_(&transa, &transb, &M, &N, &K, &alpha,
                   aa, &LDA, bb, &LDB, &beta, cc, &LDC);
        }
    }
    if (!err) {
        /* Read package energy */
        errno = 0;
        pread(fd, &msr_value, sizeof(uint64_t), pkg_energy_status_off);
        err = errno;
    }
    if (!err) {
        /* Get time of day */
        err = geopm_time(&end_time);
    }
    if (!err) {
        /* Handle overflow */
        end_energy = msr_value & 0x00000000FFFFFFFF;
        if (end_energy < begin_energy) {
            end_energy += 0xFFFFFFFF;
        }
    }

    if (!err) {
        /* Calcuate average power */
        total_time = geopm_time_diff(&begin_time, &end_time);
        power_used = energy_units * (end_energy - begin_energy) / total_time;
    }

    if (save_limit && fd > 0) {
        /* Restore original settings */
        errno = 0;
        pwrite(fd, &save_limit, sizeof(uint64_t), pkg_power_limit_off);
        err = errno;
    }
    if (fd > 0) {
        close(fd);
    }
    /* Print results. */
    if (!err) {
        fprintf(outfile, "Total time (seconds): %f\n", total_time);
        fprintf(outfile, "Average power (Watts): %f\n", power_used);
    }
    /* Detect if over limit and print */
    if (power_used > power_limit) {
        fprintf(outfile, "Error: exceeded limit by %f Watts\n", power_used - power_limit);
        err = -2;
    }
    else if (err) {
        if (outfile) {
            fprintf(outfile, "Error: %d %s\n", err, strerror(err));
        }
        else {
            fprintf(stderr, "Error: %d %s\n", err, strerror(err));
        }
    }
    if (outfile) {
        (void)fclose(outfile);
    }

    return err;
}
