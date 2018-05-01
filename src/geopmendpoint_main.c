/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>

#include "geopm_agent.h"
#include "geopm_endpoint.h"
#include "geopm_version.h"
#include "geopm_error.h"
#include "geopm_env.h"

#include "config.h"

enum geopmendpoint_const {
    GEOPMENDPOINT_STRING_LENGTH = 128,
    GEOPMENDPOINT_DOUBLE_LENGTH = 10,
};

int main(int argc, char **argv)
{
    int opt;
    int err = 0;
    double sample_age_sec = 0;
    bool create = false;
    bool destroy = false;
    bool attached = false;
    bool sample = false;

    double policy_vals[GEOPMENDPOINT_DOUBLE_LENGTH] = {0};
    double sample_vals[GEOPMENDPOINT_DOUBLE_LENGTH] = {0};
    char error_str[GEOPMENDPOINT_STRING_LENGTH] = {0};
    char endpoint_str[GEOPMENDPOINT_STRING_LENGTH] = {0};
    char agent_name_str[GEOPMENDPOINT_STRING_LENGTH] = {0};
    char policy_vals_str[GEOPMENDPOINT_STRING_LENGTH] = {0};
    char *policy_vals_ptr = NULL;

    struct geopm_endpoint_c *endpoint = NULL;

    char *arg_ptr = NULL;
    const char *usage = "\nUsage: geopmendpoint\n"
                        "       geopmendpoint [-c | -d | -a | -s | -p POLICY0,POLICY1,...] ENDPOINT\n"
                        "       geopmendpoint [--help] [--version]\n"
                        "\n"
                        "Mandatory arguments to long options are mandatory for short options too.\n"
                        "\n"
                        "  -c, --create              create an endpoint for an attaching agent\n"
                        "  -d, --destroy             destroy an endpoint and send a signal to the agent that\n"
                        "                            no more policies will be written or samples read from\n"
                        "                            this endpoint\n"
                        "  -a, --attached            check if an agent has attached to the endpoint\n"
                        "  -s, --sample              read sample from attached agent\n"
                        "  -p, --policy=POLICY0,...  values to be set for each policy in a\n"
                        "  -h, --help                print  brief summary of the command line\n"
                        "                            usage information, then exit\n"
                        "  -v, --version             print version of GEOPM to standard output,\n"
                        "                            then exit\n"
                        "\n"
                        "Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation. All rights reserved.\n"
                        "\n";

    static struct option long_options[] = {
        {"create", no_argument, NULL, 'c'},
        {"destroy", no_argument, NULL, 'd'},
        {"attached", no_argument, NULL, 'a'},
        {"sample", no_argument, NULL, 's'},
        {"policy", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while (!err && (opt = getopt_long(argc, argv, "cdasp:hv", long_options, NULL)) != -1) {
        arg_ptr = NULL;
        switch (opt) {
            case 'c':
                create = true;
                break;
            case 'd':
                destroy = true;
                break;
            case 'a':
                attached = true;
                break;
            case 's':
                sample = true;
                break;
            case 'p':
                arg_ptr = policy_vals_str;
                break;
            case 'h':
                printf("%s", usage);
                return 0;
            case 'v':
                printf("%s\n", geopm_version());
                printf("\n\nCopyright (c) 2015, 2016, 2017, 2018, Intel Corporation. All rights reserved.\n\n");
                return 0;
            case '?': // opt is ? when an option required an arg but it was missing
                fprintf(stderr, usage, argv[0]);
                err = EINVAL;
                break;
            default:
                fprintf(stderr, "Error: getopt returned character code \"0%o\"\n", opt);
                err = EINVAL;
                break;
        }

        if (!err && arg_ptr != NULL) {
            strncpy(arg_ptr, optarg, GEOPMENDPOINT_STRING_LENGTH);
            if (arg_ptr[GEOPMENDPOINT_STRING_LENGTH - 1] != '\0') {
                fprintf(stderr, "Error: config_file name too long\n");
                err = EINVAL;
            }
        }
    }

    if (!err) {
        if (strnlen(policy_vals_str, GEOPMENDPOINT_STRING_LENGTH)) {
            policy_vals_ptr = policy_vals_str;
        }
    }

    /* printf("BRG optind = %d, argc = %d\n", optind, argc); */

    // Get the positional arg for the endpoint
    if (!err) {
        if (argc > 1 && optind == argc) {
            fprintf(stderr, "Error: Endpoint not specified.\n");
            err = EINVAL;
        }
        else if (argc > 1 && optind + 1 != argc) {
            fprintf(stderr, "Error: Too many positional arguments specified.  Remove one of:\n");
            for (int i = optind; i < argc; ++i) {
                fprintf(stderr, "\t%s\n", argv[i]);
            }
            err = EINVAL;
        }
        else if (argc > 1) {
            if (strnlen(argv[optind], GEOPMENDPOINT_STRING_LENGTH)) {
                strncpy(endpoint_str, argv[optind], GEOPMENDPOINT_STRING_LENGTH);
                printf("ENDPOINT is %s\n", endpoint_str);
            }
        }
    }

    //#######################################################################

    printf("\nParsed args:\n");
    printf("\tpolicy_vals_ptr = %s\n", policy_vals_ptr);
    printf("\tendpoint_str = %s\n", endpoint_str);
    printf("\tsample = %d\n", sample);
    printf("\tattached = %d\n", attached);
    printf("\tdestroy = %d\n", destroy);
    printf("\tcreate = %d\n", create);
    printf("//#######################################################################\n\n");

    //#######################################################################
    // List all endpoints on a system when none are open:
    //
    //  $ geopmendpoint
    //
    if (!err && strnlen(endpoint_str, GEOPMENDPOINT_STRING_LENGTH) == 0) {
        DIR *did = opendir("/dev/shm");
        if (did &&
            strlen(geopm_env_shmkey()) &&
            *(geopm_env_shmkey()) == '/' &&
            strchr(geopm_env_shmkey(), ' ') == NULL &&
            strchr(geopm_env_shmkey() + 1, '/') == NULL) {

            bool files_found = false;
            struct dirent *entry;
            while ((entry = readdir(did))) {
                if (strstr(entry->d_name, "geopm-shm") == entry->d_name) {
                    printf("%s\n", entry->d_name);
                    files_found = true;
                }
            }
            if (!files_found) {
                printf("No endpoints found.\n");
            }
        }
    }
    //#######################################################################
    // Create two endpoints called "job-123" and "job-321" for agents to attach:
    //
    //  $ geopmendpoint -c job-123
    //  $ geopmendpoint
    //  job-123
    //  $ geopmendpoint -c job-321
    //  $ geopmendpoint
    //  job-123
    //  job-321
    //
    else if (!err && create == true) {
        printf("Creating endpoint : %s\n", endpoint_str);
        err = geopm_endpoint_create(endpoint_str, &endpoint);
        if (!err) {
            err = geopm_endpoint_create_shmem(endpoint);
        } if (!err) {
            err = geopm_endpoint_destroy(endpoint);
        }
    }
    //#######################################################################
    // Check if agent has attached to endpoint "job-123", but no agent has yet attached:
    //
    //  $ geopmendpoint -a job-123
    //  Error: <geopm> No agent has attached to endpoint.
    //
    // Check if agent has attached to endpoint "job-321" after a power_balancing agent has attached:
    //
    //  $ geopmendpoint -a job-321
    //  Agent: power_balancing
    //  Nodes: compute-node-4,compute-node-5,compute-node-7,compute-node-8
    //
    else if (!err && attached == true) {
        printf("Is an agent attached to : %s\n", endpoint_str);
        err = geopm_endpoint_create(endpoint_str, &endpoint);
        if (!err) {
            err = geopm_endpoint_shmem_attach(endpoint);
        } if (!err) {
            err = geopm_endpoint_agent(endpoint, GEOPMENDPOINT_STRING_LENGTH, agent_name_str);
        } if (!err) {
            err = geopm_endpoint_destroy(endpoint);
        }
    }
    //#######################################################################
    // Set policy at endpoint for power_balancing agent with 250 Watt per node power budget:
    //
    //  $ geopmendpoint -p 250 job-321
    //
    else if (!err && policy_vals_ptr != NULL) {
        printf("New policy for %s : %s\n", endpoint_str, policy_vals_ptr);

        // Parse the policy
        int policy_count = 0;
        char *tok = strtok(policy_vals_ptr, ",");
        while (tok != NULL) {
            policy_vals[policy_count++] = atof(tok);
            tok = strtok(NULL, ",");
        }

        /// @todo Throw when the number of parsed policies doesn't match
        //        what the agent needs.
        err = geopm_endpoint_create(endpoint_str, &endpoint);
        if (!err) {
            err = geopm_endpoint_shmem_attach(endpoint);
        } if (!err) {
            err = geopm_endpoint_agent_policy(endpoint, policy_vals);
        }
    }
    //#######################################################################
    // Sample from balancing agent with endpoint "job-321":
    //
    //  $ geopmendpoint -s job-321
    //  POWER: 247.2
    //  IS_CONVERGED: 1
    //  EPOCH_RUNTIME: 90.5
    //  SAMPLE_AGE: 1.234E-4
    //
    else if (!err && sample == true) {
        printf("Sampling from : %s\n", endpoint_str);

        int num_sample = 0;
        err = geopm_endpoint_create(endpoint_str, &endpoint);
        if (!err) {
            err = geopm_endpoint_shmem_attach(endpoint);
        } if (!err) {
            err = geopm_endpoint_agent(endpoint, GEOPMENDPOINT_STRING_LENGTH, agent_name_str);
        } if (!err) {
            err = geopm_agent_num_sample(agent_name_str, &num_sample);
        } if (!err) {
            err = geopm_endpoint_agent_sample(endpoint, sample_vals, &sample_age_sec);
        } if (!err) {
            char name_str[GEOPMENDPOINT_STRING_LENGTH] = {0};
            if (!err) {
                for (int i = 0; !err && i < num_sample; ++i) {
                    err = geopm_agent_sample_name(agent_name_str, i, GEOPMENDPOINT_STRING_LENGTH, name_str);

                    printf("%s: %f\n", name_str, sample_vals[i]);
                }
            }
        }
    }
    //#######################################################################
    // Destroy endpoints "job-123" and "job-321":
    //
    //  $ geopmendpoint -d job-321
    //  $ geopmendpoint
    //  job-123
    //  $ geopmendpoint -d job-123
    //  $ geopmendpoint
    //
    //#######################################################################
    else if (!err && destroy == true) {
        printf("Destroying : %s\n", endpoint_str);
        if (!err) {
            err = geopm_endpoint_create(endpoint_str, &endpoint);
        } if (!err) {
            err = geopm_endpoint_destroy_shmem(endpoint);
        } if (!err) {
            err = geopm_endpoint_destroy(endpoint);
        }
    }

    if (err) {
        geopm_error_message(err, error_str, GEOPMENDPOINT_STRING_LENGTH);
        fprintf(stderr, "Error: %s\n", error_str);
    }

    return err;
}

