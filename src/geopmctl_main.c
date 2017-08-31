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
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <mpi.h>

#include "geopm_version.h"
#include "geopm_error.h"
#include "geopm_env.h"

#include "config.h"

enum geopmctl_const {
    GEOPMCTL_STRING_LENGTH = 128,
};

int geopmctl_main(const char *policy_path, const char *sample_key);

int main(int argc, char **argv)
{
    int opt;
    int world_size = 1, my_rank = 0, i;
    int err0 = 0;
    int err_mpi = 0;
    char error_str[MPI_MAX_ERROR_STRING] = {0};
    char policy_config[GEOPMCTL_STRING_LENGTH] = {0};
    char policy_key[GEOPMCTL_STRING_LENGTH] = {0};
    char *policy_ptr = NULL;
    char sample_key[GEOPMCTL_STRING_LENGTH] = {0};
    char *arg_ptr = NULL;
    MPI_Comm comm_world = MPI_COMM_NULL;
    const char *usage = "    %s [--help] [--version]\n"
                        "              -c policy_config\n"
                        "              [-s sample_key]\n"
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
                        "       -c policy_config\n"
                        "              Policy configuration file or POSIX shared memory key  which  may\n"
                        "              be created with the geopm_policy_c(3) interface or the geopmpol‐\n"
                        "              icy(3) application.\n"
                        "\n"
                        "       -s sample_key\n"
                        "              POSIX shared memory key referencing memory  written  to  by  the\n"
                        "              compute  application  to  provide  the geopmctl application with\n"
                        "              performance profile information. See geopm_ctl_c(3) for informa‐\n"
                        "              tion  on  how to create the sample shared memory region for pro‐\n"
                        "              file feedback.\n"
                        "\n"
                        "    Copyright (C) 2015, 2016, 2017, Intel Corporation. All rights reserved.\n"
                        "\n";
    if (argc > 1 &&
        strncmp(argv[1], "--version", strlen("--version") + 1) == 0) {
        printf("%s\n", geopm_version());
        printf("\n\nCopyright (C) 2015, 2016, 2017, Intel Corporation. All rights reserved.\n\n");
        return 0;
    }
    if (argc > 1 && (
            strncmp(argv[1], "--help", strlen("--help") + 1) == 0 ||
            strncmp(argv[1], "-h", strlen("-h") + 1) == 0)) {
        printf(usage, argv[0]);
        return 0;
    }

    while (!err0 && (opt = getopt(argc, argv, "c:k:s:")) != -1) {
        arg_ptr = NULL;
        switch (opt) {
            case 'c':
                arg_ptr = policy_config;
                break;
            case 'k':
                arg_ptr = policy_key;
                break;
            case 's':
                arg_ptr = sample_key;
                break;
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
        i = MPI_MAX_ERROR_STRING;
        PMPI_Error_string(err_mpi, error_str, &i);
        fprintf(stderr, "Error: %s\n", error_str);
        err0 = err_mpi;
    }


    if (!err0 &&
        strlen(policy_config) != 0 &&
        strlen(policy_key) != 0) {
        err0 = EINVAL;
        fprintf(stderr, "Error: %s either -c and -k cannot both be specified\n", argv[0]);
    }

    if (!err0 && !my_rank) {
        if (policy_config[0]) {
            printf("    Policy config: %s\n", policy_config);
        }
        if (policy_key[0]) {
            printf("    Policy key:    %s\n", policy_key);
        }
        if (sample_key[0]) {
            printf("    Sample key:    %s\n", sample_key);
        }
        printf("\n");
    }

    if (!err0) {
        if (strlen(policy_config)) {
            policy_ptr = policy_config;
        }
        else if (strlen(policy_key)) {
            policy_ptr = policy_key;
        }
        else {
            /* reusing the policy_config buffer to pass environment variables */
            strncpy(policy_config, geopm_env_policy(),GEOPMCTL_STRING_LENGTH-1);
            policy_ptr = policy_config;
        }
        if (!my_rank) {
            err0 = geopmctl_main(policy_ptr, sample_key);
        }
        else {
            err0 = geopmctl_main(NULL, sample_key);
        }
        if (err0) {
            geopm_error_message(err0, error_str, GEOPMCTL_STRING_LENGTH);
            fprintf(stderr, "Error: %s\n", error_str);
        }
    }

    PMPI_Finalize();
    return err0;
}
