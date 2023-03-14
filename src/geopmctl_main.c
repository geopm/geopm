/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#ifdef ENABLE_MPI
#include <mpi.h>
#endif

#include "geopm_version.h"
#include "geopm_error.h"

enum geopmctl_const {
    GEOPMCTL_STRING_LENGTH = 128,
};

int geopmctl_main(void);

int main(int argc, char **argv)
{
    int opt;
    int err0 = 0;
    char error_str[NAME_MAX] = {0};
    char *arg_ptr = NULL;
    const char *usage = "    %s [--help] [--version]\n"
                        "\n"
                        "DESCRIPTION\n"
                        "       The geopmctl application runs concurrently with a computational MPI\n"
                        "       application to manage power settings on compute nodes allocated to the\n"
                        "       computation MPI application.\n"
                        "\n"
                        "OPTIONS\n"
                        "       --help\n"
                        "              Print  brief summary of the command line usage information, then\n"
                        "              exit.\n"
                        "\n"
                        "       --version\n"
                        "              Print version of geopm to standard output, then exit.\n"
                        "\n"
                        "    Copyright (c) 2015 - 2023, Intel Corporation. All rights reserved.\n"
                        "\n";
    if (argc > 1 &&
        strncmp(argv[1], "--version", strlen("--version") + 1) == 0) {
        printf("%s\n", geopm_version());
        printf("\n\nCopyright (c) 2015 - 2023, Intel Corporation. All rights reserved.\n\n");
        return 0;
    }
    if (argc > 1 && (
            strncmp(argv[1], "--help", strlen("--help") + 1) == 0 ||
            strncmp(argv[1], "-h", strlen("-h") + 1) == 0)) {
        printf(usage, argv[0]);
        return 0;
    }

    while (!err0 && (opt = getopt(argc, argv, "")) != -1) {
        arg_ptr = NULL;
        switch (opt) {
            default:
                fprintf(stderr, "Error: unknown parameter \"%c\"\n", opt);
                fprintf(stderr, usage, argv[0]);
                err0 = EINVAL;
                break;
        }
        if (!err0) {
            strncpy(arg_ptr, optarg, GEOPMCTL_STRING_LENGTH);
            if (arg_ptr[GEOPMCTL_STRING_LENGTH - 1] != '\0') {
                fprintf(stderr, "Error: config_file name too long\n");
                err0 = EINVAL;
            }
        }
    }
    if (!err0 && optind != argc) {
        fprintf(stderr, "Error: %s does not take positional arguments\n", argv[0]);
        fprintf(stderr, usage, argv[0]);
        err0 = EINVAL;
    }

#if ENABLE_MPI
    int world_size = 1, my_rank = 0;
    int err_mpi = 0;
    MPI_Comm comm_world = MPI_COMM_NULL;
    if (!err0) {
        err_mpi = PMPI_Init(&argc, &argv);
        comm_world = MPI_COMM_WORLD;
        if (!err_mpi) {
            err_mpi = PMPI_Comm_size(comm_world, &world_size);
        }
        if (!err_mpi) {
            err_mpi = PMPI_Comm_rank(comm_world, &my_rank);
        }
    }
    if (err_mpi) {
        i = NAME_MAX
        PMPI_Error_string(err_mpi, error_str, &i);
        fprintf(stderr, "Error: %s\n", error_str);
        err0 = err_mpi;
    }
#endif
    if (!err0) {
        err0 = geopmctl_main();
        if (err0) {
            geopm_error_message(err0, error_str, GEOPMCTL_STRING_LENGTH);
            fprintf(stderr, "Error: %s\n", error_str);
        }
    }

#if ENABLE_MPI
    PMPI_Finalize();
#endif
    return err0;
}
