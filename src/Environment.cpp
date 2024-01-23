/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Environment.hpp"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <algorithm>
#include <string>
#include <vector>
#include <utility>
#include <set>

#include "geopm/json11.hpp"

#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "PlatformIOProf.hpp"
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
        : EnvironmentImp(DEFAULT_CONFIG_PATH, OVERRIDE_CONFIG_PATH, nullptr)
    {

    }

    EnvironmentImp::EnvironmentImp(const std::string &default_config_path,
                                   const std::string &override_config_path,
                                   const PlatformIO *platform_io)
        : m_all_names(get_all_vars())
        , m_name_value_map ({{"GEOPM_AGENT", "monitor"},
#ifdef GEOPM_ENABLE_MPI
                             {"GEOPM_COMM" ,"MPIComm"},
#else
                             {"GEOPM_COMM" ,"NullComm"},
#endif
                             {"GEOPM_MAX_FAN_OUT", "16"},
                             {"GEOPM_TIMEOUT", "30"},
                             {"GEOPM_DEBUG_ATTACH", "-1"},
                             {"GEOPM_NUM_PROC", "1"}})
        , m_default_config_path(default_config_path)
        , m_override_config_path(override_config_path)
        , m_platform_io(platform_io)
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
            m_name_value_map["GEOPM_ENDPOINT"] = std::move(default_endpoint);
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
                "GEOPM_RECORD_FILTER",
                "GEOPM_INIT_CONTROL",
                "GEOPM_PERIOD",
                "GEOPM_NUM_PROC",
                "GEOPM_PROGRAM_FILTER",
                "GEOPM_CTL_LOCAL"};
    }

    void EnvironmentImp::parse_environment()
    {
        for (const auto &env_var : m_all_names) {
            std::string value;
            if(get_env(env_var, value)) {
                m_name_value_map[env_var] = std::move(value);
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
                              << " has been overridden with value <"  << override_value << ">" << std::endl;
                }
                name_value_map[var_name] = std::move(override_value);
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
        std::string ret = "NullComm";
        if (!do_ctl_local()) {
            ret = lookup("GEOPM_COMM");
        }
        return ret;
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

    double EnvironmentImp::period(double default_period) const
    {
        double result = default_period;
        std::string period_str = lookup("GEOPM_PERIOD");
        if (period_str.size() != 0) {
            try {
                result = std::stod(period_str);
            }
            catch (const std::invalid_argument &conv_ex) {
                throw geopm::Exception("EnvironmentImp::period(): GEOPM_PERIOD environment variable could not be converted into a double: \"" + period_str + "\"",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            catch (const std::out_of_range &range_ex) {
                throw geopm::Exception("EnvironmentImp::period(): GEOPM_PERIOD environment variable could not be converted into a double, out of range: \"" + period_str + "\"",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return result;
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
        std::string env_profile = lookup("GEOPM_PROFILE");
        std::string ret = env_profile;
        if (ret.empty()) {
            ret = "default";
        }
        else if (!ret.empty()) {
            // Sanitize the input: No carriage returns nor double quotes
            ret.erase(std::remove_if(ret.begin(), ret.end(),
                                     [](char &c) {
                                         return ( c == '\n' || c == '"');
                                     }),
                      ret.end());
        }
        if (!env_profile.empty() && ret != env_profile) {
            std::cerr << "Warning: <geopm> The GEOPM_PROFILE contains invalid characters: \""
                      << env_profile << "\" converted to \"" << ret << "\n";
        }
        ret = "\"" + ret + "\""; // Add quotes for later YAML parsing
        return ret;
    }

    std::string EnvironmentImp::frequency_map(void) const
    {
        return lookup("GEOPM_FREQUENCY_MAP");
    }

    std::vector<std::pair<std::string, int> > EnvironmentImp::trace_signals(void) const
    {
        return signal_parser(lookup("GEOPM_TRACE_SIGNALS"));
    }

    std::vector<std::pair<std::string, int> > EnvironmentImp::report_signals(void) const
    {
        return signal_parser(lookup("GEOPM_REPORT_SIGNALS"));
    }

    std::vector<std::pair<std::string, int> > EnvironmentImp::signal_parser(std::string environment_variable_contents) const
    {
        // Lazy init must be done here since the Environment singleton is used in MPI_Init
        if (m_platform_io == nullptr) {
            m_platform_io = &PlatformIOProf::platform_io();
        }

        std::vector<std::pair<std::string, int> > result_data_structure;

        auto signals_avail = m_platform_io->signal_names();
        auto individual_signals = geopm::string_split(environment_variable_contents, ",");
        for (const auto &signal : individual_signals) {
            auto signal_domain = geopm::string_split(signal, "@");
            if (signals_avail.find(signal_domain[0]) == signals_avail.end()) {
                throw Exception("Invalid signal : " + signal_domain[0],
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            if (signal_domain.size() == 2) {
                result_data_structure.push_back(std::make_pair(
                    signal_domain[0],
                    geopm::PlatformTopo::domain_name_to_type(signal_domain[1])
                ));
            }
            else if (signal_domain.size() == 1) {
                result_data_structure.push_back(std::make_pair(signal_domain[0], GEOPM_DOMAIN_BOARD));
            }
            else {
                throw Exception("EnvironmentImp::signal_parser(): Environment trace extension contains signals with multiple \"@\" characters.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }

        return result_data_structure;
    }

    int EnvironmentImp::max_fan_out(void) const
    {
        int result = 0;
        std::string fan_out_str = lookup("GEOPM_MAX_FAN_OUT");
        try {
            result = std::stoi(fan_out_str);
        }
        catch (const std::invalid_argument &conv_ex) {
            throw geopm::Exception("EnvironmentImp::max_fan_out(): GEOPM_MAX_FAN_OUT environment variable could not be converted into an integer: \"" + fan_out_str + "\"",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        catch (const std::out_of_range &range_ex) {
            throw geopm::Exception("EnvironmentImp::max_fan_out(): GEOPM_MAX_FAN_OUT environment variable could not be converted into an integer, out of range: \"" + fan_out_str + "\"",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int EnvironmentImp::pmpi_ctl(void) const
    {
        int ret = Environment::M_CTL_NONE;
        auto it = m_name_value_map.find("GEOPM_CTL");
        if (it != m_name_value_map.end() &&
            it->second != "application") {
            std::string pmpi_ctl_str f= it->second;
            if (pmpi_ctl_str == "process") {
                ret = Environment::M_CTL_PROCESS;
            }
            else if (pmpi_ctl_str == "pthread") {
                ret = Environment::M_CTL_PTHREAD;
            }
            else {
                throw Exception("EnvironmentImp::EnvironmentImp(): " + pmpi_ctl_str +
                                " is not a valid value for GEOPM_CTL see geopm(7).",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
#ifndef GEOPM_ENABLE_MPI
        if (ret == Environment::M_CTL_PROCESS ||
            ret == Environment::M_CTL_PTHREAD) {
            throw Exception("Environment: libgeopm.so was compiled without MPI support, setting GEOPM_CTL to a value other than \"application\" (the default) is invalid",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
#endif
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
        bool result = false;
        if (is_set("GEOPM_PROGRAM_FILTER")) {
            auto valid_names = geopm::string_split(lookup("GEOPM_PROGRAM_FILTER"), ",");
            result = std::any_of(valid_names.begin(), valid_names.end(),
                                 [](const std::string &vn) {
                                     return vn == program_invocation_name ||
                                            vn == program_invocation_short_name;});
        }
        return result;
    }

    int EnvironmentImp::timeout(void) const
    {
        int result = 0;
        std::string timeout_str = lookup("GEOPM_TIMEOUT");
        try {
            result = std::stoi(timeout_str);
        }
        catch (const std::invalid_argument &conv_ex) {
            throw geopm::Exception("EnvironmentImp::timeout(): GEOPM_TIMEOUT environment variable could not be converted into an integer: \"" + timeout_str + "\"",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        catch (const std::out_of_range &range_ex) {
            throw geopm::Exception("EnvironmentImp::timeout(): GEOPM_TIMEOUT environment variable could not be converted into an integer, out of range: \"" + timeout_str + "\"",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int EnvironmentImp::num_proc(void) const
    {
        return std::stoi(lookup("GEOPM_NUM_PROC"));
    }

    bool EnvironmentImp::do_ctl_local(void) const
    {
        bool result = true;
#ifdef GEOPM_ENABLE_MPI
        if (!is_set("GEOPM_CTL_LOCAL")) {
            result = false;
        }
#endif
        return result;
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

    std::string EnvironmentImp::init_control(void) const
    {
        return lookup("GEOPM_INIT_CONTROL");
    }

    bool EnvironmentImp::do_init_control(void) const
    {
        return is_set("GEOPM_INIT_CONTROL");
    }
}
