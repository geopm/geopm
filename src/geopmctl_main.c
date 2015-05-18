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
#include <unistd.h>
#include <getopt.h>

#include "geopm.h"

enum geopmctl_const {
    GEOPMCTL_MAX_FACTOR = 8,
    GEOPMCTL_STRING_LENGTH = 128,
};

static int parse_csv_int(const char *csv_string, int *num_el, int *result);

int main(int argc, char **argv)
{
    int opt;
    int world_size, my_rank, i;
    int err0 = 0;
    int err_mpi = 0;
    int num_factor = GEOPMCTL_MAX_FACTOR;
    int factor[GEOPMCTL_MAX_FACTOR] = {0};
    size_t factor_prod;
    char report[GEOPMCTL_STRING_LENGTH] = {0};
    char control[GEOPMCTL_STRING_LENGTH] = {0};
    const char *usage = "Usage: %s -f factor_list [-c control] [-r report]\n"
                        "    factor_list:   Comma separated list of tree hierarchy factors\n"
                        "                   root fan out factor first and leaf fan-out\n"
                        "                   factor last.\n"
                        "    control:       File which modifies geopmctl behavior\n"
                        "    report:        Output file that will hold the report when\n"
                        "                   program terminates\n"
                        "\n";

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (!my_rank) {
        while (!err0 && (opt = getopt(argc, argv, "hf:c:r:")) != -1) {
            switch (opt) {
                case 'f':
                    err0 = parse_csv_int(optarg, &num_factor, factor);
                    if (err0) {
                        fprintf(stderr, "ERROR: parsing factor list \"%s\"\n", optarg);
                    }
                    break;
                case 'c':
                    strncpy(control, optarg, GEOPMCTL_STRING_LENGTH);
                    if (control[GEOPMCTL_STRING_LENGTH - 1] != '\0') {
                        fprintf(stderr, "ERROR: control name too long\n");
                        err0 = -1;
                    }
                    break;
                case 'r':
                    strncpy(report, optarg, GEOPMCTL_STRING_LENGTH);
                    if (report[GEOPMCTL_STRING_LENGTH - 1] != '\0') {
                        fprintf(stderr, "ERROR: report name too long\n");
                        err0 = -1;
                    }
                    break;
                case 'h':
                    fprintf(stderr, usage, argv[0]);
                    err0 = -1;
                    break;
                default:
                    fprintf(stderr, "ERROR: unknown parameter \"%c\"\n", opt);
                    fprintf(stderr, usage, argv[0]);
                    err0 = -1;
            }
        }
        if (!err0 && optind != argc) {
            fprintf(stderr, "ERROR: %s does not take positional arguments\n", argv[0]);
            fprintf(stderr, usage, argv[0]);
            err0 = -1;
        }
        if (!err0 && factor[0] == 0) {
            fprintf(stderr, "ERROR: Flag -f is required, and factor must be non-zero\n");
            fprintf(stderr, usage, argv[0]);
            err0 = -1;
        }
        if (!err0) {
            factor_prod = 1;
            for (i = 0; i < num_factor; ++i) {
                factor_prod *= factor[i];
            }
            if (factor_prod != (size_t)world_size) {
                fprintf(stderr, "ERROR: Product of factors must equal size of MPI_COMM_WORLD\n");
                err0 = -1;
            }
        }
    }
    err_mpi = MPI_Bcast(&err0, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (err_mpi && !my_rank) {
        err0 = err_mpi;
    }
    if (!err0 && !err_mpi) {
        err_mpi = MPI_Bcast(&num_factor, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    if (!err0 && !err_mpi) {
        err_mpi = MPI_Bcast(factor, num_factor, MPI_INT, 0, MPI_COMM_WORLD);
    }
    if (!err0 && !err_mpi) {
        if (!my_rank) {
            fprintf(stdout, "factors: ");
            for (i = 0 ; i < num_factor; ++i) {
                fprintf(stdout, "%d ", factor[i]);
            }
            fprintf(stdout, "\n");
            fprintf(stdout, "control: %s\n", control);
            fprintf(stdout, "report: %s\n", report);
        }
        err0 = geopm_ctl_run(num_factor, factor, control, report, MPI_COMM_WORLD);
    }

    if (err_mpi) {
        i = GEOPMCTL_STRING_LENGTH;
        MPI_Error_string(err_mpi, control, &i);
        fprintf(stderr, "ERROR: %s\n", control);
        err0 = err_mpi;
    }
    MPI_Finalize();
    return err0;
}

static int parse_csv_int(const char *csv_string, int *num_el, int *result)
{
    int err = 0;
    int i;
    int max_num_el = *num_el;
    char *begin_ptr, *end_ptr;
    char csv_copy[GEOPMCTL_STRING_LENGTH] = {0};

    *num_el = 0;
    strncpy(csv_copy, csv_string, GEOPMCTL_STRING_LENGTH);
    if (csv_copy[GEOPMCTL_STRING_LENGTH - 1] != 0) {
        err = -1;
    }

    begin_ptr = csv_copy;
    end_ptr = csv_copy;
    for (i = 0; *begin_ptr && end_ptr && i < max_num_el; ++i) {
        end_ptr = strchr(begin_ptr, ',');
        if (end_ptr) {
            *end_ptr = '\0';
        }
        result[i] = atoi(begin_ptr);
        if (end_ptr) {
            begin_ptr = end_ptr + 1;
        }
    }
    *num_el = i;
    if (*num_el == 0) {
        err = -1;
    }
    return err;
}
