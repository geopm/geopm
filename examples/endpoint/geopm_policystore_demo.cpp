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
#include <signal.h>
#include <getopt.h>

#include <iostream>

#include "geopm_version.h"
#include "Endpoint.hpp"
#include "PolicyStore.hpp"
#include "geopm_endpoint_demo.hpp"
#include "Exception.hpp"
#include "geopm_endpoint.h"


static bool g_continue = true;
static void handler(int sig)
{
    g_continue = false;
}

/// Applying static policies will use the PolicyStore; for this to
/// work, the policy path must be set in the environment override,
/// and the SQLite policy store DB must be created for the user.
int main(int argc, char **argv)
{
    // Handle signals
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    // Parse command line
    const char *usage =
        "\n"
        "Usage: geopm_static_policy_demo [-p POLICYSTORE] [-s SHMEM_PREFIX]\n"
        "       geopm_static_policy_demo [--help] [--version]\n"
        "\n"
        "Mandatory arguments to long options are mandatory for short options too.\n"
        "\n"
        "  -p, --policystore=POLICYSTORE     location of the policystore database file\n"
        "  -s, --shmem-prefix=SHMEM_PREFIX   shmem location used for Controller's GEOPM_POLICY\n"
        "  -h, --help                        print  brief summary of the command line\n"
        "                                    usage information, then exit\n"
        "  -v, --version                     print version of GEOPM to standard output,\n"
        "                                    then exit\n"
        "\n"
        "Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n"
        "\n";

    static struct option long_options[] = {
        {"policystore", no_argument, NULL, 'p'},
        {"shmem-prefix", required_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    int err = 0;
    bool do_help = false;
    bool do_version = false;
    std::string shmem_prefix = "/geopmcd_endpoint_test";
    std::string policystore_path = "/home/drguttma/policystore.db";
    while (!err && (opt = getopt_long(argc, argv, "s:p:hv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                policystore_path.assign(optarg);
                break;
            case 's':
                shmem_prefix.assign(optarg);
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
    if (!err && optind != argc) {
        fprintf(stderr, "Error: The following positional argument(s) are in error:\n");
        while (optind < argc) {
            fprintf(stderr, "%s\n", argv[optind++]);
        }
        err = EINVAL;
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
    if (err) {
        return err;
    }

    std::cout << "Using " << policystore_path << " as policystore location." << std::endl;
    std::cout << "Using " << shmem_prefix << " as endpoint shmem prefix." << std::endl;

    geopm::ShmemEndpoint endpoint(shmem_prefix);
    endpoint.open();

    try {
        // wait for attach
        std::string agent = "";
        while (g_continue && agent == "") {
            agent = endpoint.get_agent();
        }
        std::cout << "agent = " << agent << std::endl;
        if (agent != "") {
            // apply policy from the policy store
            auto policy_store = geopm::PolicyStore::make_unique(policystore_path);
            std::string profile_name = endpoint.get_profile_name();
            std::cout << "profile = " << profile_name << std::endl;
            auto policy = policy_store->get_best(profile_name, agent);
            std::cout << "Got policy: " << policy << std::endl;
            endpoint.write_policy(policy);
        }
    }
    catch (geopm::Exception &ex) {
        std::cerr << ex.what() << std::endl;
        endpoint.close();
        throw;
    }
    endpoint.close();
    return 0;
}
