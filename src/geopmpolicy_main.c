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
#include <errno.h>
#include <pthread.h>
#include <getopt.h>

#include "geopm_policy.h"
#include "geopm_static.h"
#include "geopm_policy_message.h"

enum geopmpolicy_const {
    GEOPMPOLICY_MODE_CREATE = 0,
    GEOPMPOLICY_MODE_ENFORCE = 1,
    GEOPMPOLICY_MODE_SAVE = 2,
    GEOPMPOLICY_MODE_RESTORE = 3,
    GEOPMPOLICY_STRING_LENGTH = 128,
};

int _policy_create(char *path, char* mode, char *options);
int _policy_enforce(char *path, char *mode, char *options);
int _policy_save(char *path);
int _policy_restore(char *path);

int main(int argc, char** argv)
{
    int opt;
    int err = 0;
    int mode = 0;
    char file[GEOPMPOLICY_STRING_LENGTH] = {0};
    char mode_string[GEOPMPOLICY_STRING_LENGTH] = {0};
    char option_string[GEOPMPOLICY_STRING_LENGTH] = {0};
    char copy_string[GEOPMPOLICY_STRING_LENGTH] = {0};
    FILE *infile;
    FILE *outfile;
    char *arg_ptr = NULL;

    const char *usage = "   geopmpolicy --version | --help\n"
                        "   geopmpolicy -c -f output -m mode -d key0:value0,key1:value1...\n"
                        "   geopmpolicy -e (-f input | -m mode -d key0:value0,key1:value1...)\n"
                        "   geopmpolicy -s [-f output]\n"
                        "   geopmpolicy -r [-f input]\n"
                        "\n"
                        "   --version\n"
                        "      Print version of geopm to standard file, then exit.\n"
                        "\n"
                        "   --help\n"
                        "       Print  brief   summary  of   the  command   line  usage\n"
                        "       information, then exit.\n"
                        "\n"
                        "   -c\n"
                        "       Create a geopm(3) configurtion file, -f must be specified\n"
                        "       when using this option which gives the path to the ouptut\n"
                        "       configuration file.\n"
                        "\n"
                        "   -e\n"
                        "       Enforce a static power mode, this mode can be specified\n"
                        "       with the -m and -d options or the -f option.\n"
                        "\n"
                        "   -s\n"
                        "       Create an in MSR save state file for all MSR values that\n"
                        "       geopm(3)  may modify.  The file file can be specified\n"
                        "       with -f.\n"
                        "\n"
                        "   -r\n"
                        "       Restore the MSR values that are recored in an existing\n"
                        "       MSR save state file.  The infput file can be  specified\n"
                        "       with the -f option.\n"
                        "\n"
                        "   -m mode\n"
                        "       Power management mode, must be one of those described\n"
                        "       in the MODES section of geopmpolicy(3). The static modes do not\n"
                        "       require the geopm runtime to be running concurrently\n"
                        "       with the primary computational application, where as\n"
                        "       dynamic modes do have a runtime requirement on geopm.\n"
                        "\n"
                        "   -d key0:value0,key1:value1...\n"
                        "       Specifies a dictionary of key value pairs which modify\n"
                        "       the behavior of a mode. The key and value options for each\n"
                        "       mode are described in the MODES sections of geopmpolicy(3).\n"
                        "\n"
                        "   -f file_path\n"
                        "       When used with -c or -s file_path is an file file.  When\n"
                        "       used with -e or -r file_path is an file file.  This is a\n"
                        "       geopm(3) cogfigration file when used with -c or -e and an\n"
                        "       MSR save state file when used with -s or -r.\n"
                        "\n"
                        "     Copyright (C) 2015 Intel Corporation. All rights reserved.\n"
                        "\n";

    if (argc < 2) {
        fprintf(stderr, "ERROR: No arguments specified\n");
        fprintf(stderr, usage, argv[0]);
        return EINVAL;
    }
    if (strncmp(argv[1], "--version", strlen("--version")) == 0) {
        printf("%s\n",geopm_version());
        printf("\n\nCopyright (C) 2015 Intel Corporation. All rights reserved.\n\n");
        return 0;
    }
    if (strncmp(argv[1], "--help", strlen("--help")) == 0) {
        printf("%s\n",usage);
        return 0;
    }

    while (!err && (opt = getopt(argc, argv, "hcesrm:d:f:")) != -1) {
        arg_ptr = NULL;
        switch (opt) {
            case 'c':
                mode = GEOPMPOLICY_MODE_CREATE;
                break;
            case 'e':
                mode = GEOPMPOLICY_MODE_ENFORCE;
                break;
            case 's':
                mode = GEOPMPOLICY_MODE_SAVE;
                break;
            case 'r':
                mode = GEOPMPOLICY_MODE_RESTORE;
                break;
            case 'm':
                arg_ptr = mode_string;
                break;
            case 'd':
                arg_ptr = option_string;
                break;
            case 'f':
                arg_ptr = file;
                break;
            case 'h':
                printf("%s\n",usage);
                return 0;
            default:
                fprintf(stderr, "ERROR: unknown parameter \"%c\"\n", opt);
                fprintf(stderr, usage, argv[0]);
                err = EINVAL;
                break;
        }
        if (!err && optarg != NULL) {
            strncpy(arg_ptr, optarg, GEOPMPOLICY_STRING_LENGTH);
            if (arg_ptr[GEOPMPOLICY_STRING_LENGTH - 1] != '\0') {
                fprintf(stderr, "ERROR: option string too long\n");
                err = EINVAL;
            }
        }
    }

    if (!err && optind != argc) {
        fprintf(stderr, "ERROR: %s does not take positional arguments\n", argv[0]);
        fprintf(stderr, usage, argv[0]);
        err = EINVAL;
    }

    if (!err && (mode == GEOPMPOLICY_MODE_CREATE &&
                 (strlen(mode_string) == 0 || strlen(option_string) == 0))) {
        fprintf(stderr, "Error: In mode create, -m and -d are not optional\n");
        err = EINVAL;
    }

    if (!err && (mode == GEOPMPOLICY_MODE_ENFORCE && strlen(file) == 0 &&
                 (strlen(mode_string) == 0 || strlen(option_string) == 0))) {
        fprintf(stderr, "Error: In mode enforce, either -i or -m and -d must be specified\n");
        err = EINVAL;
    }

    if (!err && mode == GEOPMPOLICY_MODE_ENFORCE && strlen(file) == 0) {
        infile = fopen(file, "r");
        if (infile == NULL) {
            fprintf(stderr, "Error: Cannot open specified file for reading: %s\n", file);
            err = EINVAL;
        }
        fclose(infile);
    }

    if (!err && mode == GEOPMPOLICY_MODE_RESTORE) {
        if(strlen(file) == 0) {
            snprintf(file, GEOPMPOLICY_STRING_LENGTH, "/tmp/.geopm_msr_restore.log");
        }
        else {
            //Make sure we are using tempfs to keep these files local to the machine
            if (strncmp(file, "/tmp/", 5)) {
                if (strlen(file) > (GEOPMPOLICY_STRING_LENGTH - strlen("/tmp/"))) {
                    fprintf(stderr, "ERROR: Specified file path too long\n");
                    err = EINVAL;
                }
                if (!err) {
                    if (file[0] == '/') {
                        snprintf(copy_string, GEOPMPOLICY_STRING_LENGTH, "/tmp/%s", file);
                        strncpy(file, copy_string, GEOPMPOLICY_STRING_LENGTH);
                    }
                    else {
                        snprintf(copy_string, GEOPMPOLICY_STRING_LENGTH, "/tmp%s", file);
                        strncpy(file, copy_string, GEOPMPOLICY_STRING_LENGTH);
                    }
                }
            }
        }
        infile = fopen(file, "w");
        if (infile == NULL) {
            fprintf(stderr, "Error: Cannot open file for reading: %s\n", file);
            err = EINVAL;
        }
        fclose(infile);
    }


    if (!err && mode == GEOPMPOLICY_MODE_CREATE) {
        outfile = fopen(file, "w");
        if (outfile == NULL) {
            fprintf(stderr, "Error: Cannot open specified file for writing: %s\n", file);
            err = EINVAL;
        }
        fclose(outfile);
    }

    if (!err && mode == GEOPMPOLICY_MODE_SAVE) {
        if(strlen(file) == 0) {
            snprintf(file, GEOPMPOLICY_STRING_LENGTH, "/tmp/.geopm_msr_restore.log");
        }
        else {
            //Make sure we are using tempfs to keep these files local to the machine
            if (strncmp(file, "/tmp/", 5)) {
                if (strlen(file) > (GEOPMPOLICY_STRING_LENGTH - strlen("/tmp/"))) {
                    fprintf(stderr, "ERROR: Specified file path too long\n");
                    err = EINVAL;
                }
                if (!err) {
                    if (file[0] == '/') {
                        snprintf(copy_string, GEOPMPOLICY_STRING_LENGTH, "/tmp/%s", file);
                        strncpy(file, copy_string, GEOPMPOLICY_STRING_LENGTH);
                    }
                    else {
                        snprintf(copy_string, GEOPMPOLICY_STRING_LENGTH, "/tmp%s", file);
                        strncpy(file, copy_string, GEOPMPOLICY_STRING_LENGTH);
                    }
                }
            }
        }
        outfile = fopen(file, "w");
        if (outfile == NULL) {
            fprintf(stderr, "Error: Cannot open file for writing: %s\n", file);
            err = EINVAL;
        }
        fclose(outfile);
    }

    if (!err) {
        switch (mode) {
            case GEOPMPOLICY_MODE_CREATE:
                err = _policy_create(file, mode_string, option_string);
                break;
            case GEOPMPOLICY_MODE_ENFORCE:
                err = _policy_enforce(file, mode_string, option_string);
                break;
            case GEOPMPOLICY_MODE_SAVE:
                err = _policy_save(file);
                break;
            case GEOPMPOLICY_MODE_RESTORE:
                err = _policy_restore(file);
                break;
            default:
                fprintf(stderr, "Error: Invalid execution mode.\n");
                err = EINVAL;
                break;
        };
    }

    return err;
}

int _policy_create(char *path, char* mode, char *options)
{
    int err = 0;
    int mode_e = -1;
    struct geopm_policy_c *policy;
    char *key, *value;

    err = geopm_policy_create("", path, &policy);
    if (err) {
        fprintf(stderr, "Error, could not create geopm_policy\n");
        return err;
    }

    if (strncmp(mode, "tdp_balance_static", strlen("tdp_balance_static")) == 0) {
        mode_e = GEOPM_MODE_TDP_BALANCE_STATIC;
    }
    else if (strncmp(mode, "freq_uniform_static", strlen("freq_uniform_static")) == 0) {
        mode_e = GEOPM_MODE_FREQ_UNIFORM_STATIC;
    }
    else if (strncmp(mode, "freq_hybrid_static", strlen("freq_hybrid_static")) == 0) {
        mode_e = GEOPM_MODE_FREQ_HYBRID_STATIC;
    }
    else if (strncmp(mode, "perf_balance_dynamic", strlen("perf_balance_dynamic")) == 0) {
        mode_e = GEOPM_MODE_PERF_BALANCE_DYNAMIC;
    }
    else if (strncmp(mode, "freq_uniform_dynamic", strlen("freq_uniform_dynamic")) == 0) {
        mode_e = GEOPM_MODE_FREQ_UNIFORM_DYNAMIC;
    }
    else if (strncmp(mode, "freq_hybrid_dynamic", strlen("freq_hybrid_dynamic")) == 0) {
        mode_e = GEOPM_MODE_FREQ_HYBRID_DYNAMIC;
    }
    else {
        fprintf(stderr, "Error: Invalid power mode: %s\n", mode);
        err = EINVAL;
    }

    if (!err ) {
        err = geopm_policy_mode(policy, mode_e);
    }

    key = strtok(options, ":");
    do {
        value = strtok(NULL, ",");
        if (value == NULL) {
            fprintf(stderr, "Error: Invalid execution mode.\n");
            err = EINVAL;
        }
        if (!err) {
            if(strncmp(key, "percent_tdp", strlen("percent_tdp")) == 0) {
                err = geopm_policy_percent_tdp(policy, atoi(value));
            }
            else if(strncmp(key, "cpu_mhz", strlen("cpu_mhz")) == 0) {
                err = geopm_policy_cpu_freq(policy, atoi(value));
            }
            else if(strncmp(key, "num_cpu_max_perf", strlen("num_cpu_max_perf")) == 0) {
                err = geopm_policy_full_perf(policy, atoi(value));
            }
            else if(strncmp(key, "affinity", strlen("affinity")) == 0) {
                if (strncmp(value, "compact", strlen("compact")) == 0) {
                    err = geopm_policy_affinity(policy, GEOPM_FLAGS_BIG_CPU_TOPOLOGY_COMPACT);
                }
                else if (strncmp(value, "scatter", strlen("scatter")) == 0) {
                    err = geopm_policy_affinity(policy, GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER);
                }
                else {
                    fprintf(stderr, "Error: invalid affinity value: %s\n", value);
                    err = EINVAL;
                }
            }
            else if(strncmp(key, "power_budget", strlen("power_budget")) == 0) {
                err = geopm_policy_power(policy, atoi(value));
            }
            else {
                fprintf(stderr, "Error: invalid option: %s\n", key);
                err = EINVAL;
            }
        }
    }
    while (!err && ((key = strtok(NULL, ":")) != NULL));

    err = geopm_policy_write(policy);

    return err;
}

int _policy_enforce(char *path, char *mode, char *options)
{
    int err = 0;

    if (strlen(path) == 0) {
        err = _policy_create("/tmp/geomp_policy.conf", mode, options);
        if (err) {
            fprintf(stderr, "Error: Could not create policy file\n");
        }
        if (!err) {
            err = staticpm_ctl_enforce("/tmp/geomp_policy.conf");
            if (err) {
                fprintf(stderr, "Error: Could not enforce policy\n");
            }
        }
    }
    else {
        err = staticpm_ctl_enforce(path);
        if (err) {
            fprintf(stderr, "Error: Could not enforce policy\n");
        }
    }

    return err;
}

int _policy_save(char *path)
{
    int err = 0;

    err = staticpm_ctl_save(path);

    return err;
}

int _policy_restore(char *path)
{
    int err = 0;

    err = staticpm_ctl_restore(path);

    return err;
}
