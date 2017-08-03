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

int rapl_platform_limit_test(double power_limit, int num_rep);

int rapl_platform_limit_test(double power_limit, int num_rep)
{
    const int matrix_size = MATRIX_SIZE;
    const int pad_size = PAD_SIZE;
    const size_t num_elements = matrix_size * (matrix_size + pad_size);
    const off_t platform_power_unit_off = 0x606;
    const off_t platform_power_limit_off = 0x65c;
    const off_t platform_energy_status_off = 0x64d;
    int err = 0;
    int msr_fd = 0;
    double power_units = 0.0;
    double energy_units = 0.0;
    double power_used = 0.0;
    double total_time = 0.0;
    uint64_t msr_value = 0;
    uint64_t save_limit = 0;
    uint64_t begin_energy = 0;
    uint64_t end_energy = 0;
    ssize_t num_byte = 0;
    double *aa = NULL;
    double *bb = NULL;
    double *cc = NULL;
    struct geopm_time_s begin_time = {{0,0}};
    struct geopm_time_s end_time = {{0,0}};
    char hostname[NAME_MAX] = {0};
    char outfile_name[NAME_MAX] = {0};
    FILE *outfile = NULL;

    if (!err) {
        /* Get hostname to insert in output file name */
        err = gethostname(hostname, NAME_MAX);
        if (err) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        int outlen = snprintf(outfile_name, NAME_MAX, "rapl_platform_limit_test_%s.out", hostname);
        if (outlen >= NAME_MAX) {
            err = ENAMETOOLONG;
        }
    }
    if (!err) {
        /* Pipe all output to a file named after the host and append if it exists */
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
        /* setup matrix values */
        memset(cc, 0, num_elements * sizeof(double));
        for (int i = 0; i < num_elements; ++i) {
            aa[i] = 1.0;
            bb[i] = 2.0;
        }
        /* Open msr device, try to use msr_safe if available */
        msr_fd = open("/dev/cpu/0/msr_safe", O_RDWR);
        if (msr_fd == -1) {
            errno = 0;
            msr_fd = open("/dev/cpu/0/msr", O_RDWR);
        }
        if (msr_fd == -1) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        /* Read RAPL_POWER_UNIT */
        errno = 0;
        num_byte = pread(msr_fd, &msr_value, sizeof(uint64_t), platform_power_unit_off);
        if (num_byte != sizeof(uint64_t)) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        power_units = pow(2.0, -1.0 * (msr_value & 0xF));
        energy_units = pow(2.0, -1.0 * ((msr_value >> 8) & 0x1F));
        /* Read RAPL_PLATFORM_POWER_LIMIT to use for restore */
        errno = 0;
        num_byte = pread(msr_fd, &msr_value, sizeof(uint64_t), platform_power_limit_off);
        if (num_byte != sizeof(uint64_t)) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        save_limit = msr_value;
        msr_value &= 0xFFFFFFFFFFFF0000;
        msr_value |= 0x000000000000FFFF & (uint64_t)(power_limit / power_units);
        /* Write RAPL_PLATFORM_POWER_LIMIT given on command line */
        errno = 0;
        num_byte = pwrite(msr_fd, &msr_value, sizeof(uint64_t), platform_power_limit_off);
        if (num_byte != sizeof(uint64_t)) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        /* Get time of day */
        err = geopm_time(&begin_time);
    }
    if (!err) {
        /* Read package energy */
        errno = 0;
        num_byte = pread(msr_fd, &msr_value, sizeof(uint64_t), platform_energy_status_off);
        if (num_byte != sizeof(uint64_t)) {
            err = errno ? errno : -1;
        }
    }
    if (!err) {
        begin_energy = msr_value & 0x00000000FFFFFFFF;

        /* Run dgemm in a loop */
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
        num_byte = pread(msr_fd, &msr_value, sizeof(uint64_t), platform_energy_status_off);
        if (num_byte != sizeof(uint64_t)) {
            err = errno ? errno : -1;
        }
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
    if (save_limit && msr_fd > 0) {
        /* Restore original settings */
        errno = 0;
        num_byte = pwrite(msr_fd, &save_limit, sizeof(uint64_t), platform_power_limit_off);
        if (num_byte != sizeof(uint64_t)) {
            err = errno;
        }
    }
    if (msr_fd > 0) {
        close(msr_fd);
    }
    if (!err) {
        /* Print results. */
        fprintf(outfile, "Total time (seconds): %f\n", total_time);
        fprintf(outfile, "Average power (Watts): %f\n", power_used);
    }
    /* Pipe error messages to standard error if they occured before
       the outfile was opened. */
    if (!outfile) {
        outfile = stderr;
    }
    /* Detect if over limit and print */
    if (power_used > power_limit) {
        fprintf(outfile, "Error: exceeded limit by %f Watts\n", power_used - power_limit);
        err = -2;
    }
    else if (err) {
        fprintf(outfile, "Error: %d %s\n", err, strerror(err));
    }
    if (outfile != stderr) {
        (void)fclose(outfile);
    }

    return err;
}

int main(int argc, char **argv)
{
    const char *usage = "%s package_limit_watts [num_rep]\n"
                        "    default num_rep=10\n";
    int err = 0;
    double power_limit = 0.0;
    int num_rep = NUM_REP;
    /* print usage */
    if (argc < 2 ||
        !strncmp(argv[1], "--help", strlen("--help")) ||
        !strncmp(argv[1], "-h", strlen("-h"))) {
        fprintf(stderr, usage, argv[0]);
        err = -1;
    }
    if (!err) {
        /* Read limit in watts from command line */
        power_limit = atof(argv[1]);
        if (power_limit <= 0.0) {
            err = EDOM;
        }
    }
    if (!err) {
        /* Read number of repetitions from command line */
        if (argc == 3) {
            num_rep = atoi(argv[2]);
            if (num_rep <= 0) {
                err = EDOM;
            }
        }
    }
    if (!err) {
        err = rapl_platform_limit_test(power_limit, num_rep);
    }
    return err;
}
