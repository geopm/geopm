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
#include <errno.h>
#include <mpi.h>

#include "geopm_version.h"

enum geopmctl_const {
    GEOPMCTL_STRING_LENGTH = 128,
};

int geopmctl_main(const char *policy_config, const char *policy_key, const char *sample_key, const char *report);

int main(int argc, char **argv)
{
    int opt;
    int world_size = 1, my_rank = 0, i;
    int err0 = 0;
    int err_mpi = 0;
    char error_str[GEOPMCTL_STRING_LENGTH] = {0};
    char policy_config[GEOPMCTL_STRING_LENGTH] = {0};
    char policy_key[GEOPMCTL_STRING_LENGTH] = {0};
    char sample_key[GEOPMCTL_STRING_LENGTH] = {0};
    char report[GEOPMCTL_STRING_LENGTH] = {0};
    char *arg_ptr = NULL;
    MPI_Comm comm_world = MPI_COMM_NULL;
    const char *usage = "     geopmctl [--version] [--help] [-c policy_config]\n"
                        "              [-k policy_key] [-s sample_key] [-r report]\n"
                        "\n"
                        "     --version\n"
                        "          Print version of geopm to standard output, then exit.\n"
                        "\n"
                        "     --help\n"
                        "          Print  brief   summary  of   the  command   line  usage\n"
                        "          information, then exit.\n"
                        "\n"
                        "     -c policy_config\n"
                        "          Policy configuration file which may be created with the\n"
                        "          geopm_policy_c(3)   interface  or   the  geopmpolicy(3)\n"
                        "          application.  If -c is specified  then -k should not be\n"
                        "          specified.  If both  are specified -k is  ignored and a\n"
                        "          warning is printed.\n"
                        "\n"
                        "     -k policy_key\n"
                        "          POSIX  shared memory  key which  determines policy  for\n"
                        "          geopmctl.  See geopm_policy_c(3) for information on how\n"
                        "          to  create  and  modify  a  shared  memory  region  for\n"
                        "          control.\n"
                        "\n"
                        "     -s sample_key\n"
                        "          POSIX shared  memory key referencing memory  written to\n"
                        "          by  the compute  application  to  provide the  geopmctl\n"
                        "          application with performance  profile information.  See\n"
                        "          geopm_ctl_c(3)  for information  on how  to create  the\n"
                        "          sample shared memory region for profile feedback.\n"
                        "\n"
                        "     -r report\n"
                        "          Output  text file  that  will hold  the human  readable\n"
                        "          report when program terminates.\n"
                        "\n"
                        "     Copyright (C) 2015 Intel Corporation. All rights reserved.\n"
                        "\n";


    if (argc >= 1 &&
        strncmp(argv[1], "--version", strlen("--version")) == 0) {
        printf("%s\n", geopm_version());
        printf("\n\nCopyright (C) 2015 Intel Corporation. All rights reserved.\n\n");
        return 0;
    }
    if (argc >= 1 && (
            strncmp(argv[1], "--help", strlen("--help")) == 0 ||
            strncmp(argv[1], "-h", strlen("-h")))) {
        printf("%s\n", usage);
        return 0;
    }

    while (!err0 && (opt = getopt(argc, argv, "c:k:s:r:")) != -1) {
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
            case 'r':
                arg_ptr = report;
                break;
            default:
                fprintf(stderr, "ERROR: unknown parameter \"%c\"\n", opt);
                fprintf(stderr, usage, argv[0]);
                err0 = EINVAL;
                break;
        }
        if (!err0) {
            strncpy(arg_ptr, optarg, GEOPMCTL_STRING_LENGTH);
            if (arg_ptr[GEOPMCTL_STRING_LENGTH - 1] != '\0') {
                fprintf(stderr, "ERROR: config_file name too long\n");
                err0 = EINVAL;
            }
        }
    }
    if (!err0 && optind != argc) {
        fprintf(stderr, "ERROR: %s does not take positional arguments\n", argv[0]);
        fprintf(stderr, usage, argv[0]);
        err0 = EINVAL;
    }

    if (!err0) {
        err_mpi = MPI_Init(&argc, &argv);
        comm_world = MPI_COMM_WORLD;
        if (!err_mpi) {
            err_mpi = MPI_Comm_size(comm_world, &world_size);
        }
        if (!err_mpi) {
            err_mpi = MPI_Comm_rank(comm_world, &my_rank);
        }
    }
    if (err_mpi) {
        i = GEOPMCTL_STRING_LENGTH;
        MPI_Error_string(err_mpi, error_str, &i);
        fprintf(stderr, "ERROR: %s\n", error_str);
        err0 = err_mpi;
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
        if (report[0]) {
            printf("    Report file:   %s\n", report);
        }
        printf("\n");
    }
    err0 = geopmctl_main(policy_config, policy_key, sample_key, report);

    MPI_Finalize();
    return err0;
}
