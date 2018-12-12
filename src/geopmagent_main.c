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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#include "geopm_agent.h"
#include "geopm_version.h"
#include "geopm_error.h"
#include "geopm_env.h"

#include "config.h"

enum geopmagent_const {
    GEOPMAGENT_STRING_LENGTH = 512,
    GEOPMAGENT_DOUBLE_LENGTH = 100,
};

int main(int argc, char **argv)
{
    int opt;
    int err = 0;
    int output_num;
    double policy_vals[GEOPMAGENT_DOUBLE_LENGTH] = {0};
    char error_str[GEOPMAGENT_STRING_LENGTH] = {0};
    char agent_str[GEOPMAGENT_STRING_LENGTH] = {0};
    char policy_vals_str[GEOPMAGENT_STRING_LENGTH] = {0};
    char output_str[NAME_MAX * GEOPMAGENT_DOUBLE_LENGTH];
    char *agent_ptr = NULL;
    char *policy_vals_ptr = NULL;
    char *arg_ptr = NULL;
    const char *usage = "\nUsage: geopmagent\n"
                        "       geopmagent [-a AGENT] [-p POLICY0,POLICY1,...]\n"
                        "       geopmagent [--help] [--version]\n"
                        "\n"
                        "Mandatory arguments to long options are mandatory for short options too.\n"
                        "\n"
                        "  -a, --agent=AGENT         specify the name of the agent\n"
                        "  -p, --policy=POLICY0,...  values to be set for each policy in a\n"
                        "                            comma-seperated list\n"
                        "  -h, --help                print  brief summary of the command line\n"
                        "                            usage information, then exit\n"
                        "  -v, --version             print version of GEOPM to standard output,\n"
                        "                            then exit\n"
                        "\n"
                        "Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation. All rights reserved.\n"
                        "\n";

    static struct option long_options[] = {
        {"agent", required_argument, NULL, 'a'},
        {"policy", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while (!err && (opt = getopt_long(argc, argv, "a:p:hv", long_options, NULL)) != -1) {
        arg_ptr = NULL;
        switch (opt) {
            case 'a':
                arg_ptr = agent_str;
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

        if (!err) {
            strncpy(arg_ptr, optarg, GEOPMAGENT_STRING_LENGTH);
            if (arg_ptr[GEOPMAGENT_STRING_LENGTH - 1] != '\0') {
                fprintf(stderr, "Error: config_file name too long\n");
                err = EINVAL;
            }
        }
    }

    if (!err && optind != argc) {
        fprintf(stderr, "Error: The following positional argument(s) are in error:\n");
        while (optind < argc) {
            fprintf(stderr, "%s\n", argv[optind++]);
        }
        err = EINVAL;
    }

    if (!err) {
        if (strnlen(agent_str, GEOPMAGENT_STRING_LENGTH)) {
            agent_ptr = agent_str;
        }
        if (strnlen(policy_vals_str, GEOPMAGENT_STRING_LENGTH)) {
            policy_vals_ptr = policy_vals_str;
        }
    }

    if (!err && argc == 1) {
        err = geopm_agent_num_avail(&output_num);
        if (!err) {
            for(int i = 0; !err && i < output_num; ++i) {
                err = geopm_agent_name(i, sizeof(output_str), output_str);
                if (!err) {
                    printf("%s\n", output_str);
                }
            }
        }
    }
    else if (!err && agent_ptr != NULL && policy_vals_ptr == NULL) {
        err = geopm_agent_supported(agent_ptr);

        // Policies
        if (!err) {
            err = geopm_agent_num_policy(agent_ptr, &output_num);
        }

        if (!err) {
            printf("Policy: ");
            for (int i = 0; !err && i < output_num; ++i) {
                err = geopm_agent_policy_name(agent_ptr, i, sizeof(output_str), output_str);
                if (!err) {
                    if (i > 0) {
                        printf(",");
                    }
                    printf("%s", output_str);
                }
            }
            if (!err && output_num == 0) {
                printf("(none)\n");
            }
            else {
                printf("\n");
            }
        }

        // Samples
        if (!err) {
            err = geopm_agent_num_sample(agent_ptr, &output_num);
        }

        if (!err) {
            printf("Sample: ");
            for (int i = 0; !err && i < output_num; ++i) {
                err = geopm_agent_sample_name(agent_ptr, i, sizeof(output_str), output_str);
                if (!err) {
                    if (i > 0) {
                        printf(",");
                    }
                    printf("%s", output_str);
                }
            }
            if (!err && output_num == 0) {
                printf("(none)\n");
            }
            else {
                printf("\n");
            }
        }
    }
    else if (!err) {

        if (agent_ptr == NULL) {
            fprintf(stderr, "Error: Agent (-a) must be specified to create a policy.\n");
            err = EINVAL;
        }

        int num_policy = 0;
        if (!err) {
            err = geopm_agent_num_policy(agent_ptr, &num_policy);
        }

        if (!err) {
            if (num_policy) {
                int policy_count = 0;
                char *tok = strtok(policy_vals_ptr, ",");
                while (!err && tok != NULL) {
                    /// @todo atof returns 0.0 if this string is invalid or None
                    policy_vals[policy_count++] = atof(tok);
                    if (policy_count > GEOPMAGENT_DOUBLE_LENGTH) {
                        err = E2BIG;
                    }
                    tok = strtok(NULL, ",");
                }
                if (!err && num_policy != policy_count) {
                    fprintf(stderr, "Error: Number of policies read from command line does not match agent.\n");
                    err = EINVAL;
                }
            }
            else if (strncmp(policy_vals_ptr, "none", 4) != 0 &&
                     strncmp(policy_vals_ptr, "None", 4) != 0) {
                fprintf(stderr, "Error: Must specify \"None\" for the parameter option if agent takes no parameters.\n");
                err = EINVAL;
            }
        }
        if(!err) {
            err = geopm_agent_policy_json(agent_ptr, policy_vals, sizeof(output_str), output_str);
            printf("%s\n", output_str);
        }
    }

    if (err) {
        geopm_error_message(err, error_str, GEOPMAGENT_STRING_LENGTH);
        fprintf(stderr, "Error: %s\n", error_str);
    }

    return err;
}
