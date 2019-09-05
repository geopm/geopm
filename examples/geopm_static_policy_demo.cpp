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

#include "Endpoint.hpp"
#include "PolicyStore.hpp"

/// Helpers for printing
template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> vec)
{
    os << "{";
    for (int ii = 0; ii < vec.size(); ++ii) {
        os << vec[ii];
        if (ii < vec.size() - 1) {
            os << ", ";
        }
    }
    os << "}";
    return os;
}

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
    /// Handle signals
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    geopm::ShmemEndpoint endpoint("/geopm_endpoint_test");
    endpoint.open();
    // wait for attach
    std::string agent = "";
    while (g_continue && agent == "") {
        agent = endpoint.get_agent();
    }
    if (agent != "") {
        /// todo: handle exceptions
        auto policy_store = geopm::PolicyStore::make_unique("/home/drguttma/policystore.db");
        std::string profile_name = endpoint.get_profile_name();
        auto policy = policy_store->get_best(profile_name, agent);
        std::cout << "Got policy: " << policy << std::endl;
        endpoint.write_policy(policy);
    }
    endpoint.close();
    return 0;
}
