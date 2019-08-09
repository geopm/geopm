/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include <iostream>
#include <fstream>

#include "geopm_time.h"
#include "geopm_version.h"
#include "Endpoint.hpp"
#include "Agent.hpp"
#include "Helper.hpp"
#include "PolicyStore.hpp"
#include "geopm_endpoint_demo.hpp"

/// This daemon is the short-term example compute node daemon.  In the future
/// it will connect to a head node daemon to receive commands and send samples.
/// Currently, the behavior is:
/// - wait for a Controller agent to attach
/// - load an initial policy from policy store
/// - continuously read samples and log to a file
/// - accept SIGUSR1 as an indication to reload policy from policy store
/// - accept SIGUSR2 as an indication to print a summary of the
///     current endpoint contents
/// - accept SIGINT to shutdown gracefully
/// - detect agent detach or timeout, and being waiting for a new agent
/// - command line arguments are used to specify location of the policy store
///     and endpoint shared memory name
/// NOT in scope:
/// - socket connection to head node daemon (later)
/// - dynamic policy algorithm (later)
/// - one-off commands to set policy from command line (this is
///     the  purpose of geopmendpoint)
/// - file policy instead of shared memory (not a use case we plan to support)
/// - configuration of daemon from file (later)
/// - high availability and security features (ongoing)
///

static bool g_continue = true;
static void handler(int sig)
{
    if (sig == SIGINT) {
        g_continue = false;
    }
}


int main(int argc, char **argv)
{
    /// Handle signals
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    /// Handle command line arguments
    const char *usage = "\n"
        "Usage: geopmcd\n";
    static struct option long_options[] = {
        {"policystore", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    int err = 0;
    int opt;
    char *arg_ptr = NULL;
    bool do_help = false;
    bool do_version = false;
    std::string shmem_prefix = "/geopmcd_endpoint_test";
    while (!err && (opt = getopt_long(argc, argv, "p:hv", long_options, NULL)) != -1) {
        arg_ptr = NULL;
        switch (opt) {
            case 'p':
                shmem_prefix.assign(arg_ptr)
                break;
            case 'h':
                do_help = true;
                break;
            case 'v':
                do_version = true;
                break;
            case '?': // opt is ? when an option required an arg but it was missing
                do_help = true;
                err = EINVAL;
                break;
            default:
                fprintf(stderr, "Error: getopt returned character code \"0%o\"\n", opt);
                err = EINVAL;
                break;
        }
    }

    if (do_help) {
        printf("%s", usage);
    }
    if (do_version) {
        printf("%s\n", geopm_version());
        printf("\n\nCopyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n\n");
    }
    if (do_help || do_version) {
        return err;
    }


    auto endpoint = std::make_shared<geopm::ShmemEndpoint>(shmem_prefix);
    while (g_continue) {
        endpoint->open();

        // check for agent
        std::string agent = "";
        std::string profile_name;
        while (agent == "") {
            agent = endpoint->get_agent();
        }
        std::cout << "Controller with agent " << agent << " attached." << std::endl;

        // initial policy
        auto policy_store = geopm::PolicyStore::make_unique("/home/drguttma/policystore.db");
        profile_name = endpoint->get_profile_name();
        auto policy = policy_store->get_best(profile_name, agent);
        std::cout << "Got policy: " << policy << std::endl;
        endpoint->write_policy(policy);

        // todo: check daemon mode here to avoid early return above
        std::ofstream log_file("endpoint_test.log");
        std::vector<double> sample(geopm::Agent::num_sample(geopm::agent_factory().dictionary(agent)));

        while (agent != "") {


            void get_sample_or_timeout(struct geopm_time_s *last_sample_time,
                                       std::string *agent)
            {
                const geopm_time_s zero {{0, 0}};
                const double TIMEOUT = 3.0;
                geopm_time_s sample_time;
                geopm_time_s current_time;
                do {
                    geopm_time(&current_time);
                    sample_time = endpoint->read_sample(sample);
                }
                while (geopm_time_diff(&sample_time, &zero) == 0.0 ||
                       (geopm_time_diff(&sample_time, &last_sample_time) == 0.0 &&
                        geopm_time_diff(&last_sample_time, &current_time) < TIMEOUT));

                if (geopm_time_diff(&last_sample_time, &current_time) >= TIMEOUT) {
                    std::cerr << "Timeout waiting for Controller sample." << std::endl;
                    agent = "";
                    endpoint->close();
                }
                else {
                    *last_sample_time = sample_time;
                    log_file << sample_time << " " << sample << std::endl;
                    *agent = endpoint->get_agent();
                }
            }


            struct geopm_time_s last_sample_time;
            geopm_time(&last_sample_time);

            while (g_continue && agent != "") {
                static geopm_time_s start_time;
                geopm_time(&start_time);

                write_dynamic_power_policy(endpoint, start_time);

                get_sample_or_timeout(&last_sample_time, &agent);

            }
            std::cout << "Controller detached." << std::endl;
        } // endwhile g_continue
        endpoint->close();
    } // endif is_daemon
    else {
        if (view_agent) {
            // Display agent, profile, node list
            auto agent = endpoint->
        }

    }
    return 0;
}
