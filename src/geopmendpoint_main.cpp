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

#include "Endpoint.hpp"
#include "OptionParser.hpp"
#include "config.h"


// todo: will require feature of shared memory/endpoint allowing
// it to take over an existed shared memory region created
// by a different process
// Feature to print all endpoints requires some common prefix
// for all endpoint keys.
int main(int argc, char *argv[])
{
    geopm::OptionParser parser("geopmendpoint", std::cout, std::cerr, "");
    parser.add_example_usage(""); // no args
    parser.add_option("create", 'c', "create", false,
                      "create an endpoint for an attaching agent");
    parser.add_option("destroy", 'd', "destroy", false,
                      "destroy an endpoint and signal to the agent that no more "
                      "policies will be written or samples read from this endpoint");
    parser.add_option("attached", 'a', "attached", false,
                      "check if an agent has attached to the endpoint");
    parser.add_option("profile", 'f', "profile", false,
                      "read profile name from attached agent");
    parser.add_option("nodes", 'n', "nodes", false,
                      "read list of nodes in attached job");
    parser.add_option("sample", 's', "sample", false,
                      "read sample from attached agent");
    parser.add_option("policy", 'p', "policy", "",
                      "values to be set for each policy in a comma-separated list");
    parser.add_example_usage("[-c | -d | -a | -f | -n | -s | -p POLICY0,POLICY1,...] ENDPOINT");

    bool early_exit = parser.parse(argc, argv);
    // required positional argument
    auto pos_args = parser.get_positional_args();
    if (!early_exit && pos_args.size() != 1) {
        // todo: print all endpoints, unless any other option was passed, then error
        std::cerr << "Endpoint name is required." << std::endl;
        // do help?
    }
    if (early_exit) {
        return 0;
    }

    return 0;
}
