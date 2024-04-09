/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#include <iostream>

#include "geopm_agent.h"
#include "geopm_error.h"
#include "geopm_hash.h"
#include "geopm/Agent.hpp"
#include "OptionParser.hpp"

#include "config.h"

enum geopmagent_const {
    GEOPMAGENT_STRING_LENGTH = 512,
    GEOPMAGENT_DOUBLE_LENGTH = 100,
};

static int main_imp(int argc, char **argv);

int main(int argc, char **argv)
{
    int err = 0;
    try {
        err = main_imp(argc, argv);
    }
    catch (const geopm::Exception &ex) {
        std::cerr << "Error: geopmagent: " << ex.what() << "\n\n";
        err = ex.err_value();
    }
    return err;
}

static int main_imp(int argc, char **argv)
{
    geopm::OptionParser parser{"geopmagent", std::cout, std::cerr, ""};
    parser.add_option("agent", 'a', "agent", "", "specify the name of the agent");
    parser.add_option("policy", 'p', "policy", "",
                      "values to be set for each policy in a comma-separated list");
    parser.add_example_usage("");
    parser.add_example_usage("[-a AGENT] [-p POLICY0,POLICY1,...]");
    bool early_exit = parser.parse(argc, argv);
    if (early_exit) {
        return 0;
    }

    int err = 0;
    auto pos_args = parser.get_positional_args();
    if (pos_args.size() > 0) {
        std::cerr << "Error: The following positional argument(s) are in error:" << std::endl;
        for (const std::string &arg : pos_args) {
            std::cerr << arg << std::endl;
        }
        err = EINVAL;
    }

    int output_num;
    double policy_vals[GEOPMAGENT_DOUBLE_LENGTH] = {0};
    char error_str[GEOPMAGENT_STRING_LENGTH] = {0};
    char agent_str[GEOPMAGENT_STRING_LENGTH] = {0};
    char policy_vals_str[GEOPMAGENT_STRING_LENGTH] = {0};
    char output_str[NAME_MAX * GEOPMAGENT_DOUBLE_LENGTH];
    const char *agent_ptr = NULL;
    char *policy_vals_ptr = NULL;

    try {
        std::string agent = parser.get_value("agent");
        if (agent != "") {
            strncpy(agent_str, agent.c_str(), GEOPMAGENT_STRING_LENGTH - 1);
            agent_ptr = agent_str;
        }
        std::string policy = parser.get_value("policy");
        if (parser.get_value("policy") != "") {
            strncpy(policy_vals_str, policy.c_str(), GEOPMAGENT_STRING_LENGTH - 1);
            policy_vals_ptr = policy_vals_str;
        }
    }
    catch (...)
    {
        err = geopm::exception_handler(std::current_exception(), false);
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

        int policy_count = 0;
        if (!err) {
            if (num_policy) {
                char *tok = strtok(policy_vals_ptr, ",");
                char *endptr = NULL;
                while (!err && tok != NULL) {
                    policy_vals[policy_count] = strtod(tok, &endptr);
                    if (tok == endptr) {
                        std::string policy_name = geopm::Agent::policy_names(agent_ptr).at(policy_count);
                        if (policy_name.find("HASH") != std::string::npos ||
                            policy_name.find("hash") != std::string::npos) {
                            policy_vals[policy_count] = geopm_crc32_str(tok);
                        }
                        else {
                            fprintf(stderr, "Error: %s is not a valid floating-point number; "
                                            "use \"NAN\" to indicate default.\n", tok);
                            err = EINVAL;
                        }
                    }
                    ++policy_count;
                    if (policy_count > GEOPMAGENT_DOUBLE_LENGTH) {
                        err = E2BIG;
                    }
                    tok = strtok(NULL, ",");
                }
                if (!err && policy_count > num_policy) {
                    fprintf(stderr, "Error: Number of policies read from command line is greater than expected for agent.\n");
                    err = EINVAL;
                }
            }
            else if (strncmp(policy_vals_ptr, "none", 4) != 0 &&
                     strncmp(policy_vals_ptr, "None", 4) != 0) {
                fprintf(stderr, "Error: Must specify \"None\" for the parameter option if agent takes no parameters.\n");
                err = EINVAL;
            }
        }
        if (!err) {
            err = geopm_agent_policy_json_partial(agent_ptr, policy_count, policy_vals,
                                                  sizeof(output_str), output_str);
            printf("%s\n", output_str);
        }
    }

    if (err) {
        geopm_error_message(err, error_str, GEOPMAGENT_STRING_LENGTH);
        fprintf(stderr, "Error: %s\n", error_str);
    }

    return err;
}
