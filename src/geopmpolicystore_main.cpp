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

#include <iostream>

#include "OptionParser.hpp"
#include "Agent.hpp"
#include "PolicyStore.hpp"
#include "Helper.hpp"
#include "config.h"

// todo: move to helper
#include "examples/endpoint/geopm_endpoint_demo.hpp"

int main(int argc, char *argv[])
{
    geopm::OptionParser parser {"geopmpolicystore", std::cout, std::cerr, ""};
    // common parameters
    parser.add_option("database", 'd', "database", "/opt/geopm/policystore.db",
                      "location of the policystore database");
    parser.add_example_usage("-d DATABASE");
    parser.add_option("agent", 'a', "agent", "",
                      "name of the agent to read or write policies for");
    parser.add_option("profile", 'p', "profile", "",
                      "name of the profile to read or write policies for");
    parser.add_example_usage("-d DATABASE [-a AGENT [-p PROFILE] ]");

    // set values
    parser.add_option("policy", 'P', "policy", "",
                      "set the default policy for the given agent and optional profile as a comma-separated list of values");
    parser.add_example_usage("-d DATABASE [-a AGENT [-p PROFILE] -P POLICY0,POLICY1,...]");

    // delete values
    parser.add_option("delete_policy", 'D', "delete-policy", false,
                      "delete default or profile-specific policies for the given agent and profile.");
    parser.add_example_usage("-d DATABASE -D [-a AGENT [-p PROFILE] ]");

    bool early_exit = parser.parse(argc, argv);
    if (early_exit) {
        return 0;
    }
    std::string db_loc = parser.get_value("database");
    std::string agent = parser.get_value("agent");
    std::string profile = parser.get_value("profile");
    std::string policy = parser.get_value("policy");
    bool delete_policy = parser.is_set("delete_policy");


    std::cout << "Using db: " << db_loc << std::endl;
    std::cout << "Agent: " << agent << std::endl;
    std::cout << "Profile: " << profile << std::endl;
    std::cout << "Policy: " << policy << std::endl;
    std::cout << "do delete: " << delete_policy << std::endl;


    auto store = geopm::PolicyStore::make_unique(db_loc);
    auto agents = geopm::agent_factory().plugin_names();


    if (delete_policy) {
        if (profile == "") {
            store->set_default(agent, {});
        }
        else {
            store->set_best(profile, agent, {});
        }
    }
    //todo: error checking
    if (policy != "") {
        auto pieces = geopm::string_split(policy, ",");
        std::vector<double> pol;
        for (const auto &pp : pieces) {
            // todo: check all versions
            // todo: special handling for monitor
            if (pp == "NAN") {
                pol.push_back(NAN);
            }
            else {
                pol.push_back(std::stod(pp));
            }
        }
        if (profile != "") {
            store->set_best(profile, agent, pol);
        }
        else {
            store->set_default(agent, pol);
        }
    }

    if (agent != "") {
        if (profile != "") {
            std::cout << "Best policy for \"" << profile << "\" with " << agent << " agent:" << std::endl;
            std::cout << store->get_best(profile, agent) << std::endl;
        }
        else {
            std::cout << "Default policy for " << agent << " agent:" << std::endl;
            std::cout << store->get_best(profile, agent) << std::endl;
        }
    }
    else {
        std::cout << "Default policies: " << std::endl;
        for (const auto &ag : agents) {
            std::cout << ag << "\t" << store->get_best(profile, ag) << std::endl;
        }
    }

    // todo: no way to list all stored profile names

    return 0;
}
