/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "config.h"

#include <cmath>
#include <cpuid.h>
#include <memory>
#include <sstream>

#include "Admin.hpp"
#include "Agent.hpp"
#include "Environment.hpp"
#include "Exception.hpp"
#include "FilePolicy.hpp"
#include "Helper.hpp"
#include "MSRIOGroup.hpp"
#include "OptionParser.hpp"


namespace geopm
{
    Admin::Admin()
        : Admin(geopm::environment().default_config_path(),
                geopm::environment().override_config_path(),
                MSRIOGroup::cpuid())
    {

    }

    Admin::Admin(const std::string &default_config_path,
                 const std::string &override_config_path,
                 int cpuid_local)
        : m_default_config_path(default_config_path)
        , m_override_config_path(override_config_path)
        , m_cpuid_local(cpuid_local)
    {

    }

    void Admin::main(int argc, const char **argv, std::ostream &std_out, std::ostream &std_err)
    {
        OptionParser par = parser(std_out, std_err);
        bool early_exit = par.parse(argc, argv);
        if (!early_exit) {
            auto pos_args = par.get_positional_args();
            if (!pos_args.empty()) {
                throw geopm::Exception("The following positional argument(s) are in error: " +
                                       geopm::string_join(pos_args, " "),
                                       EINVAL, __FILE__, __LINE__);
            }
            std_out << run(par.is_set("default"),
                           par.is_set("override"),
                           par.is_set("whitelist"),
                           std::stoi(par.get_value("cpuid"), NULL, 16));
        }
    }

    std::string Admin::run(bool do_default,
                           bool do_override,
                           bool do_whitelist,
                           int cpuid)
    {
        int action_count = 0;
        action_count += do_default;
        action_count += do_override;
        action_count += do_whitelist;
        if (action_count > 1) {
            throw geopm::Exception("geopmadmin: -d, -o and -w must be used exclusively",
                                   EINVAL, __FILE__, __LINE__);
        }

        std::string result;
        if (do_default) {
            result = default_config();
        }
        else if (do_override) {
            result = override_config();
        }
        else if (do_whitelist) {
            result = whitelist(cpuid);
        }
        else {
            result = check_node();
        }
        return result;
    }

    OptionParser Admin::parser(std::ostream &std_out, std::ostream &std_err)
    {
        OptionParser result{"geopmadmin", std_out, std_err, ""};
        result.add_option("default", 'd', "config-default", false,
                          "print the path of the GEOPM default configuration file");
        result.add_option("override", 'o', "config-override", false,
                          "print the path of the GEOPM override configuration file");
        result.add_option("whitelist", 'w', "msr-whitelist", false,
                          "print the minimum msr-safe whitelist required by GEOPM");
        result.add_option("cpuid", 'c', "cpuid", "-1",
                          "cpuid in hexidecimal for whitelist (default is current platform)");
        result.add_example_usage("");
        result.add_example_usage("[--config-default|--config-override|--msr-whitelist] [--cpuid]");
        return result;
    }

    std::string Admin::default_config(void)
    {
        return m_default_config_path + "\n";
    }

    std::string Admin::override_config(void)
    {
        return m_override_config_path + "\n";
    }

    std::string Admin::whitelist(int cpuid)
    {
        if (cpuid == -1) {
            cpuid = m_cpuid_local;
        }
        return geopm::MSRIOGroup::msr_whitelist(cpuid);
    }

    std::vector<std::string> Admin::dup_keys(const std::map<std::string, std::string> &map_a,
                                             const std::map<std::string, std::string> &map_b)
    {
        std::vector<std::string> result;
        auto a_it = map_a.begin();
        auto b_it = map_b.begin();
        while (a_it != map_a.end() && b_it != map_b.end()) {
            if (a_it->first == b_it->first) {
                result.push_back(a_it->first);
                ++a_it;
                ++b_it;
            }
            else if (a_it->first < b_it->first) {
                ++a_it;
            }
            else {
                ++b_it;
            }
        }
        return result;
    }

    std::string Admin::check_node(void)
    {
        std::map<std::string, std::string> default_map =
            Environment::parse_environment_file(m_default_config_path);
        std::map<std::string, std::string> override_map =
            Environment::parse_environment_file(m_override_config_path);
        // Check for parameters that are defined in both files
        std::vector<std::string> overlap = dup_keys(default_map, override_map);
        if (!overlap.empty()) {
            throw Exception("Admin::check_node: "
                            "parameter(s) defined in both the override and default files: \"" +
                             string_join(overlap, "\", \"") + "\"\n",
                             EINVAL, __FILE__, __LINE__);
        }
        // Combine settings
        std::map<std::string, std::string> config_map(override_map);
        config_map.insert(default_map.begin(), default_map.end());
        // Check configuration and print it out
        std::vector<std::string> policy_names;
        std::vector<double> policy_vals;
        check_config(config_map, policy_names, policy_vals);
        return print_config(config_map, override_map, policy_names, policy_vals);
    }

    void Admin::check_config(const std::map<std::string, std::string> &config_map,
                             std::vector<std::string> &policy_names,
                             std::vector<double> &policy_vals)
    {
        if (config_map.empty()) {
            throw Exception("Admin::check_node(): Configuration files do not exist or are empty",
                            ENOENT, __FILE__, __LINE__);
        }
        // Determine if there is an agent set and check the policy
        auto agent_it = config_map.find("GEOPM_AGENT");
        auto policy_it = config_map.find("GEOPM_POLICY");
        if (agent_it != config_map.end()) {
            std::string agent_name = agent_it->second;
            policy_names = Agent::policy_names(agent_name);
            if (policy_it != config_map.end()) {
                FilePolicy file_policy(policy_it->second, policy_names);
                policy_vals = file_policy.get_policy();
            }
            else {
                policy_vals.resize(policy_names.size(), NAN);
            }
            std::unique_ptr<Agent> agent = agent_factory().make_plugin(agent_name);
            agent->validate_policy(policy_vals);
        }
        else if (policy_it != config_map.end()) {
            throw Exception("Admin::check_node(): A policy was specified, but not an agent",
                            EINVAL, __FILE__, __LINE__);
        }
    }

    std::string Admin::print_config(const std::map<std::string, std::string> &config_map,
                                    const std::map<std::string, std::string> &override_map,
                                    const std::vector<std::string> &policy_names,
                                    const std::vector<double> &policy_vals)
    {
        std::ostringstream result;
        result << "GEOPM CONFIGURATION\n"
               << "===================\n\n";
        // ensure all override vals are set
        std::map<std::string, std::string> config_map_copy = config_map;
        for (const auto &override_config: override_map) {
            config_map_copy[override_config.first] = override_config.second;
        }
        for (const auto &config: config_map_copy) {
            result << "    " << config.first << "=" << config.second;
            if (override_map.find(config.first) != override_map.end()) {
                result << " (override)\n";
            }
            else {
                result << " (default)\n";
            }
        }
        if (!policy_vals.empty()) {
            result << "\n";
            result << "AGENT POLICY\n"
                   << "============\n\n";
            auto name_it = policy_names.begin();
            auto val_it = policy_vals.begin();
            while (name_it != policy_names.end() &&
                   val_it != policy_vals.end()) {
                result << "    " << *name_it << "=" << *val_it << "\n";
                ++name_it;
                ++val_it;
            }
        }
        return result.str();
    }
}
