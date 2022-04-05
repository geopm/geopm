/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#include "Environment.hpp"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>
#include <set>

#include "geopm/json11.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_prof.h"

#include "config.h"

using json11::Json;

namespace geopm
{
    static const std::string DEFAULT_CONFIG_PATH = GEOPM_CONFIG_PATH "/environment-default.json";
    static const std::string OVERRIDE_CONFIG_PATH = GEOPM_CONFIG_PATH "/environment-override.json";

    static EnvironmentImp &test_environment(void)
    {
        static EnvironmentImp instance;
        return instance;
    }

    const Environment &environment(void)
    {
        return test_environment();
    }

    static bool get_env(const std::string &name, std::string &env_string)
    {
        bool result = false;
        char *check_string = getenv(name.c_str());
        if (check_string != NULL) {
            env_string = check_string;
            result = true;
        }
        return result;
    }

    std::map<std::string, std::string> Environment::parse_environment_file(const std::string &env_file_path)
    {
        std::map<std::string, std::string> ret;
        EnvironmentImp::parse_environment_file(env_file_path , EnvironmentImp::get_all_vars(), {}, ret);
        return ret;
    }

    EnvironmentImp::EnvironmentImp()
        : EnvironmentImp(DEFAULT_CONFIG_PATH, OVERRIDE_CONFIG_PATH)
    {

    }

    EnvironmentImp::EnvironmentImp(const std::string &default_config_path, const std::string &override_config_path)
        : m_all_names(get_all_vars())
        , m_runtime_names({"GEOPM_PROFILE",
                           "GEOPM_REPORT",
                           "GEOPM_TRACE",
                           "GEOPM_TRACE_PROFILE",
                           "GEOPM_CTL"})
        , m_name_value_map ({{"GEOPM_COMM" ,"MPIComm"},
                             {"GEOPM_AGENT", "monitor"},
                             {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
                             {"GEOPM_MAX_FAN_OUT", "16"},
                             {"GEOPM_TIMEOUT", "30"},
                             {"GEOPM_DEBUG_ATTACH", "-1"}})
        , m_default_config_path(default_config_path)
        , m_override_config_path(override_config_path)
    {
        parse_environment_file(m_default_config_path, m_all_names, m_user_defined_names, m_name_value_map);
        // Special handling for GEOPM_POLICY and
        // GEOPM_ENDPOINT: If user provides GEOPM_POLICY
        // through environment or command line args,
        // GEOPM_ENDPOINT from the default environment only
        // should be disabled.  GEOPM_ENDPOINT can still be
        // overridden through later override settings.
        std::string default_endpoint = endpoint();
        bool have_default_endpoint = is_set("GEOPM_ENDPOINT");
        if (have_default_endpoint) {
            m_name_value_map.erase("GEOPM_ENDPOINT");
        }
        parse_environment();
        if (have_default_endpoint && !is_set("GEOPM_ENDPOINT") && !is_set("GEOPM_POLICY")) {
            // restore default endpoint only if user did not pass GEOPM_POLICY
            m_name_value_map["GEOPM_ENDPOINT"] = default_endpoint;
        }
        parse_environment_file(m_override_config_path, m_all_names, m_user_defined_names, m_name_value_map);
    }

    std::set<std::string> EnvironmentImp::get_all_vars()
    {
        return {"GEOPM_CTL",
                "GEOPM_REPORT",
                "GEOPM_REPORT_SIGNALS",
                "GEOPM_COMM",
                "GEOPM_POLICY",
                "GEOPM_ENDPOINT",
                "GEOPM_AGENT",
                "GEOPM_SHMKEY",
                "GEOPM_TRACE",
                "GEOPM_TRACE_SIGNALS",
                "GEOPM_TRACE_PROFILE",
                "GEOPM_TRACE_ENDPOINT_POLICY",
                "GEOPM_TIMEOUT",
                "GEOPM_DEBUG_ATTACH",
                "GEOPM_PROFILE",
                "GEOPM_FREQUENCY_MAP",
                "GEOPM_MAX_FAN_OUT",
                "GEOPM_OMPT_DISABLE",
                "GEOPM_RECORD_FILTER"};
    }

    void EnvironmentImp::parse_environment()
    {
        for (const auto &env_var : m_all_names) {
            std::string value;
            if(get_env(env_var, value)) {
                m_name_value_map[env_var] = value;
                m_user_defined_names.insert(env_var);
            }
        }
    }

    void EnvironmentImp::parse_environment_file(const std::string &settings_path,
                                                const std::set<std::string> &all_names,
                                                const std::set<std::string> &user_defined_names,
                                                std::map<std::string, std::string> &name_value_map)
    {
        std::string json_str;
        bool good_path = true;
        try {
            json_str = read_file(settings_path);
        }
        catch (const geopm::Exception &ex) {
            //@todo specific exception?
            good_path = false;
        }
        if (good_path) {
            std::string err;
            Json json_root = Json::parse(json_str, err);
            if (!err.empty() || !json_root.is_object()) {
                throw Exception("EnvironmentImp::" + std::string(__func__) +
                                "(): detected a malformed json config file: " + err,
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            for (const auto &json_obj : json_root.object_items()) {
                std::string var_name = json_obj.first;
                auto var_it = all_names.find(var_name);
                if (var_it == all_names.end()) {
                    throw Exception("EnvironmentImp::" + std::string(__func__) +
                                    ": environment key " + var_name + " is unexpected",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                else if (json_obj.second.type() != Json::STRING) {
                    throw Exception("EnvironmentImp::" + std::string(__func__) +
                                    ": value for " + var_name + " expected to be a string",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                auto override_value = json_obj.second.string_value();
                if (user_defined_names.find(var_name) != user_defined_names.end()) {
                    auto user_value = name_value_map[var_name];
                    std::cerr << "Warning: <geopm> User provided environment variable \"" << var_name
                              << "\" with value <"  << user_value << ">"
                              << " has been overriden with value <"  << override_value << ">" << std::endl;
                }
                name_value_map[var_name] = override_value;
            }
        }
    }

    bool EnvironmentImp::is_set(const std::string &env_var) const
    {
        return m_name_value_map.find(env_var) != m_name_value_map.end();
    }

    std::string EnvironmentImp::lookup(const std::string &env_var) const
    {
        std::string ret;
        auto it = m_name_value_map.find(env_var);
        if (it != m_name_value_map.end()) {
            ret = it->second;
        }
        return ret;
    }

    std::string EnvironmentImp::report(void) const
    {
        return lookup("GEOPM_REPORT");
    }

    std::string EnvironmentImp::comm(void) const
    {
        return lookup("GEOPM_COMM");
    }

    std::string EnvironmentImp::policy(void) const
    {
        return lookup("GEOPM_POLICY");
    }

    std::string EnvironmentImp::endpoint(void) const
    {
        return lookup("GEOPM_ENDPOINT");
    }

    std::string EnvironmentImp::agent(void) const
    {
        return lookup("GEOPM_AGENT");
    }

    std::string EnvironmentImp::shmkey(void) const
    {
        std::string ret = lookup("GEOPM_SHMKEY");
        if (ret[0] != '/') {
            ret.insert(0, "/");
        }
        return ret;
    }

    std::string EnvironmentImp::trace(void) const
    {
        return lookup("GEOPM_TRACE");
    }

    std::string EnvironmentImp::trace_profile(void) const
    {
        return lookup("GEOPM_TRACE_PROFILE");
    }

    std::string EnvironmentImp::trace_endpoint_policy(void) const
    {
        return lookup("GEOPM_TRACE_ENDPOINT_POLICY");
    }

    std::string EnvironmentImp::profile(void) const
    {
        std::string ret = lookup("GEOPM_PROFILE");
        if (do_profile() && ret.empty()) {
            ret = std::string(program_invocation_name);
        }
        return ret;
    }

    std::string EnvironmentImp::frequency_map(void) const
    {
        return lookup("GEOPM_FREQUENCY_MAP");
    }

    std::string EnvironmentImp::trace_signals(void) const
    {
        return lookup("GEOPM_TRACE_SIGNALS");
    }

    std::string EnvironmentImp::report_signals(void) const
    {
        return lookup("GEOPM_REPORT_SIGNALS");
    }

    int EnvironmentImp::max_fan_out(void) const
    {
        return std::stoi(lookup("GEOPM_MAX_FAN_OUT"));
    }

    int EnvironmentImp::pmpi_ctl(void) const
    {
        int ret = GEOPM_CTL_NONE;
        auto it = m_name_value_map.find("GEOPM_CTL");
        if (it != m_name_value_map.end()) {
            std::string pmpi_ctl_str = it->second;
            if (pmpi_ctl_str == "process") {
                ret = GEOPM_CTL_PROCESS;
            }
            else if (pmpi_ctl_str == "pthread") {
                ret = GEOPM_CTL_PTHREAD;
            }
            else {
                throw Exception("EnvironmentImp::EnvironmentImp(): " + pmpi_ctl_str +
                                " is not a valid value for GEOPM_CTL see geopm(7).",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return ret;
    }

    bool EnvironmentImp::do_policy(void) const
    {
        return is_set("GEOPM_POLICY");
    }

    bool EnvironmentImp::do_endpoint(void) const
    {
        return is_set("GEOPM_ENDPOINT");
    }

    bool EnvironmentImp::do_trace(void) const
    {
        return is_set("GEOPM_TRACE");
    }

    bool EnvironmentImp::do_trace_profile(void) const
    {
        return is_set("GEOPM_TRACE_PROFILE");
    }

    bool EnvironmentImp::do_trace_endpoint_policy(void) const
    {
        return is_set("GEOPM_TRACE_ENDPOINT_POLICY");
    }

    bool EnvironmentImp::do_profile(void) const
    {

        return std::any_of(m_runtime_names.begin(), m_runtime_names.end(),
                           [this](std::string var) {return (is_set(var));});
    }

    int EnvironmentImp::timeout(void) const
    {
        return std::stoi(lookup("GEOPM_TIMEOUT"));
    }

    bool EnvironmentImp::do_debug_attach_all(void) const
    {
        bool result = false;
        if (is_set("GEOPM_DEBUG_ATTACH")) {
            try {
                std::stoi(lookup("GEOPM_DEBUG_ATTACH"));
            }
            catch (const std::exception &) {
                result = true;
            }
        }
        return result;
    }

    bool EnvironmentImp::do_debug_attach_one(void) const
    {
        bool result = false;
        if (is_set("GEOPM_DEBUG_ATTACH")) {
            try {
                std::stoi(lookup("GEOPM_DEBUG_ATTACH"));
                result = true;
            }
            catch (const std::exception &) {

            }
        }
        return result;
    }

    int EnvironmentImp::debug_attach_process(void) const
    {
        return std::stoi(lookup("GEOPM_DEBUG_ATTACH"));
    }

    bool EnvironmentImp::do_ompt(void) const
    {
        return !is_set("GEOPM_OMPT_DISABLE");
    }

    std::string EnvironmentImp::default_config_path(void) const
    {
        return m_default_config_path;
    }

    std::string EnvironmentImp::override_config_path(void) const
    {
        return m_override_config_path;
    }

    std::string EnvironmentImp::record_filter(void) const
    {
        return lookup("GEOPM_RECORD_FILTER");
    }

    bool EnvironmentImp::do_record_filter(void) const
    {
        return is_set("GEOPM_RECORD_FILTER");
    }
}
