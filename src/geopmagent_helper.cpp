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

#include "geopmagent_helper.hpp"

#include <algorithm>
#include <string>

#include "OptionParser.hpp"
#include "Agent.hpp"
#include "Helper.hpp"

namespace geopm
{
    static std::string string_to_lower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return str;
    }

    int geopmagent_helper(int argc, const char **argv,
                           std::ostream &stdout, std::ostream &stderr)
    {
        int err = 0;
        geopm::OptionParser parser{"geopmagent", stdout, stderr, ""};
        parser.add_option("agent", 'a', "agent", "", "specify the name of the agent");
        parser.add_option("policy", 'p', "policy", "",
                          "values to be set for each policy in a comma-separated list");
        parser.add_example_usage("");
        parser.add_example_usage("[-a AGENT] [-p POLICY0,POLICY1,...]");
        bool early_exit = parser.parse(argc, argv);
        if (early_exit) {
            return 0;
        }

        std::string agent = parser.get_value("agent");
        std::string policy = parser.get_value("policy");

        auto pos_args = parser.get_positional_args();
        if (pos_args.size() > 0) {
            stderr << "Error: The following positional argument(s) are in error:" << std::endl;
            for (const std::string &arg : pos_args) {
                stderr << arg << std::endl;
            }
            err = EINVAL;
        }

        if (agent != "" && policy == "") {
            // @todo: function: print policy and sample names
            auto policy_names = Agent::policy_names(agent);
            stdout << "Policy: ";
            if (policy_names.size() == 0) {
                stdout << "(none)";
            }
            for (const auto &policy : policy_names) {
                if (policy != *policy_names.begin()) {
                    stdout << ",";
                }
                stdout << policy;
            }
            stdout << "\n";
            auto sample_names = Agent::sample_names(agent);
            stdout << "Sample: ";
            if (sample_names.size() == 0) {
                stdout << "(none)";
            }
            for (const auto &sample : sample_names) {
                if (sample != *sample_names.begin()) {
                    stdout << ",";
                }
                stdout << sample;
            }
            stdout << "\n";

        }
        else if (agent != "" && policy != "") {
            policy = string_to_lower(policy);
            auto policy_names = Agent::policy_names(agent);
            if (policy_names.size() == 0 && policy == "none") {
                stdout << "{}\n";
            }
            else if (policy_names.size() == 0) {
                stderr << "Error: Must specify \"None\" for the parameter option if agent takes no parameters.\n";
                err = EINVAL;
            }
            else { // policy_names.size() > 0
                // split on commas
                auto values = string_split(policy, ",");
                if (values.size() > policy_names.size()) {
                    // todo: error
                    //stderr << "Error: Number of policies read from command line is greater than expected for agent.\n";
                }
                else {
                    std::vector<double> vals(values.size());
                    for (int pp = 0; pp < values.size(); ++pp) {
                        try {
                            vals[pp] = std::stod(values[pp]);
                        }
                        catch (std::invalid_argument&) {
                            stderr << "Error: " << values[pp] << " is not a valid floating-point number; "
                                   << "use \"NAN\" to indicate default.\n";
                        }
                    }
                    // todo: use helper from Policy patch...
                }
            }
        }
        else {
            auto all_agents = geopm::agent_factory().plugin_names();
            for (const auto &agent : all_agents) {
                stdout << agent << std::endl;
            }
        }

        return err;
    }
}
