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

#include <iostream>

#include "geopm_version.h"
#include "geopm_endpoint.h"
#include "geopm_endpoint_demo.hpp"
#include "Endpoint.hpp"
#include "PolicyStore.hpp"
#include "Exception.hpp"
#include "OptionParser.hpp"

static bool g_continue = true;
static void handler(int sig)
{
    g_continue = false;
}

/// Applying static policies will use the PolicyStore; for this to
/// work, the policy path must be set in the environment override or
/// on the geopmlaunch command line and the SQLite policy store DB
/// must be created for the user.
int main(int argc, char **argv)
{
    // Handle signals
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    // Parse command line
    std::string policystore_path = "/home/drguttma/policystore.db";
    std::string shmem_prefix = "/geopm_endpoint_test";
    geopm::OptionParser parser("geopm_policystore_demo", std::cout, std::cerr, "");
    parser.add_option("policystore", 'p', "policystore", policystore_path,
                      "location of the policystore database file");
    parser.add_option("shmem_prefix", 's', "shmem-prefix", shmem_prefix,
                      "shmem location used for Controller's GEOPM_POLICY");
    parser.add_example_usage("[-p POLICYSTORE] [-s SHMEM_PREFIX]");

    bool early_exit = parser.parse(argc, argv);
    auto pos_args = parser.get_positional_args();
    if (pos_args.size() > 0) {
        std::cerr << "Error: The following positional argument(s) are in error:\n";
        for (const auto &pa : pos_args) {
            std::cerr << pa << "\n";
        }
        return EINVAL;
    }

    if (early_exit) {
        return 0;
    }
    policystore_path = parser.get_value("policystore");
    shmem_prefix = parser.get_value("shmem_prefix");

    std::cout << "Using " << policystore_path << " as policystore location." << std::endl;
    std::cout << "Using " << shmem_prefix << " as endpoint shmem prefix." << std::endl;

    // Set up Endpoint
    auto endpoint = geopm::Endpoint::make_unique(shmem_prefix);
    endpoint->open();
    try {
        // wait for attach from controller
        std::string agent = "";
        while (g_continue && agent == "") {
            agent = endpoint->get_agent();
        }
        if (agent != "") {
            // apply policy from the policy store
            auto policy_store = geopm::PolicyStore::make_unique(policystore_path);
            std::string profile_name = endpoint->get_profile_name();
            std::cout << "profile = " << profile_name << std::endl;
            auto policy = policy_store->get_best(profile_name, agent);
            std::cout << "Got policy: " << policy << std::endl;
            endpoint->write_policy(policy);
        }
    }
    catch (geopm::Exception &ex) {
        endpoint->close();
        throw;
    }
    endpoint->close();
    return 0;
}
