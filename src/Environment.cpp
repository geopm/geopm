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

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
const char *program_invocation_name = "geopm_profile";
#endif

#include "Environment.hpp"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <set>

#include "contrib/json11/json11.hpp"

#include "geopm_env.h"
#include "geopm_internal.h"
#include "Exception.hpp"
#include "Helper.hpp"

#include "config.h"

using json11::Json;

namespace geopm
{
    static const std::string DEFAULT_SETTINGS_PATH = "/etc/geopm/environment-default.json";
    static const std::string OVERRIDE_SETTINGS_PATH = "/etc/geopm/environment-override.json";

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
            if (strlen(check_string) > NAME_MAX) {
                throw Exception("EnvironmentImp::EnvironmentImp(): Environment variable too long",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            env_string = check_string;
            result = true;
        }
        return result;
    }

    static void parse_environment(const std::set<std::string> &known_env_variables,
                                  std::map<std::string, std::string> &str_settings)
    {
        for (const auto &env_var : known_env_variables) {
            std::string value;
            if(get_env(env_var, value)) {
                str_settings[env_var] = value;
            }
        }
    }

    static void parse_environment_file(const std::set<std::string> &known_env_variables,
                                       const std::string &settings_path,
                                       std::map<std::string, std::string> &str_settings)
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
            Json root = Json::parse(json_str, err);
            if (!err.empty() || !root.is_object()) {
                throw Exception("EnvironmentImp::" + std::string(__func__) + "(): detected a malformed json config file: " + err,
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
            for (const auto &obj : root.object_items()) {
                if (known_env_variables.find(obj.first) == known_env_variables.end()) {
                    throw Exception("EnvironmentImp::" + std::string(__func__) +
                                    ": environment key " + obj.first + " is unexpected",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                else if (obj.second.type() != Json::STRING) {
                    throw Exception("EnvironmentImp::" + std::string(__func__) +
                                    ": value for " + obj.first + " expected to be a string",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                str_settings[obj.first] = obj.second.string_value();
            }
        }
    }

    EnvironmentImp::EnvironmentImp()
        : EnvironmentImp(DEFAULT_SETTINGS_PATH, OVERRIDE_SETTINGS_PATH)
    {
    }

    EnvironmentImp::EnvironmentImp(const std::string &default_settings_path, const std::string &override_settings_path)
        : m_known_vars({"GEOPM_CTL",
                        "GEOPM_REPORT",
                        "GEOPM_COMM",
                        "GEOPM_POLICY",
                        "GEOPM_AGENT",
                        "GEOPM_SHMKEY",
                        "GEOPM_TRACE",
                        "GEOPM_PLUGIN_PATH",
                        "GEOPM_REGION_BARRIER",
                        "GEOPM_TIMEOUT",
                        "GEOPM_DEBUG_ATTACH",
                        "GEOPM_PROFILE",
                        "GEOPM_FREQUENCY_MAP",
                        "GEOPM_MAX_FAN_OUT",
                        "GEOPM_TRACE_SIGNALS",
                        "GEOPM_REPORT_SIGNALS",
                    })
    {
        load(default_settings_path, override_settings_path);
    }

    void EnvironmentImp::load(void)
    {
        load(DEFAULT_SETTINGS_PATH, OVERRIDE_SETTINGS_PATH);
    }

    void EnvironmentImp::load(const std::string &default_settings_path, const std::string &override_settings_path)
    {
        m_vars = {{"GEOPM_COMM" ,"MPIComm"},
                  {"GEOPM_AGENT", "monitor"},
                  {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
                  {"GEOPM_MAX_FAN_OUT", "16"},
                  {"GEOPM_TIMEOUT", "30"},
                  {"GEOPM_DEBUG_ATTACH", "-1"},
                 };
        parse_environment_file(m_known_vars, default_settings_path, m_vars);

        parse_environment(m_known_vars, m_vars);
        parse_environment_file(m_known_vars, override_settings_path, m_vars);
    }

    bool EnvironmentImp::is_set(const std::string &env_var) const
    {
        return m_vars.find(env_var) != m_vars.end();
    }

    std::string EnvironmentImp::at_env(const std::string &env_var) const
    {
        std::string ret;
        auto it = m_vars.find(env_var);
        if (it != m_vars.end()) {
            ret = it->second;
        }
        return ret;
    }

    std::string EnvironmentImp::report(void) const
    {
        return at_env("GEOPM_REPORT");
    }

    std::string EnvironmentImp::comm(void) const
    {
        return at_env("GEOPM_COMM");
    }

    std::string EnvironmentImp::policy(void) const
    {
        return at_env("GEOPM_POLICY");
    }

    std::string EnvironmentImp::agent(void) const
    {
        return at_env("GEOPM_AGENT");
    }

    std::string EnvironmentImp::shmkey(void) const
    {
        std::string ret = at_env("GEOPM_SHMKEY");
        if (ret[0] != '/') {
            ret.insert(0, "/");
        }
        return ret;
    }

    std::string EnvironmentImp::trace(void) const
    {
        return at_env("GEOPM_TRACE");
    }

    std::string EnvironmentImp::profile(void) const
    {
        std::string ret = at_env("GEOPM_PROFILE");
        if (do_profile() && !ret.length()) {
            ret = std::string(program_invocation_name);
        }
        return ret;
    }

    std::string EnvironmentImp::frequency_map(void) const
    {
        return at_env("GEOPM_FREQUENCY_MAP");
    }

    std::string EnvironmentImp::plugin_path(void) const
    {
        return at_env("GEOPM_PLUGIN_PATH");
    }

    std::string EnvironmentImp::trace_signals(void) const
    {
        return at_env("GEOPM_TRACE_SIGNALS");
    }

    std::string EnvironmentImp::report_signals(void) const
    {
        return at_env("GEOPM_REPORT_SIGNALS");
    }

    int EnvironmentImp::max_fan_out(void) const
    {
        return std::stoi(at_env("GEOPM_MAX_FAN_OUT"));
    }

    int EnvironmentImp::pmpi_ctl(void) const
    {
        int ret = GEOPM_CTL_NONE;
        auto it = m_vars.find("GEOPM_CTL");
        if (it != m_vars.end()) {
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

    bool EnvironmentImp::do_region_barrier(void) const
    {
        return is_set("GEOPM_REGION_BARRIER");
    }

    bool EnvironmentImp::do_trace(void) const
    {
        return is_set("GEOPM_TRACE");
    }

    bool EnvironmentImp::do_profile(void) const
    {
        return is_set("GEOPM_PROFILE") ||
               is_set("GEOPM_REPORT") ||
               is_set("GEOPM_TRACE") ||
               is_set("GEOPM_CTL");
    }

    int EnvironmentImp::timeout(void) const
    {
        return std::stoi(at_env("GEOPM_TIMEOUT"));
    }

    int EnvironmentImp::debug_attach(void) const
    {
        return std::stoi(at_env("GEOPM_DEBUG_ATTACH"));
    }
}

extern "C"
{
    void geopm_env_load(void)
    {
        geopm::test_environment().load();
    }

    int geopm_env_pmpi_ctl(int *pmpi_ctl)
    {
        int err = 0;
        try {
            if (!pmpi_ctl) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *pmpi_ctl = geopm::environment().pmpi_ctl();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }

    int geopm_env_do_profile(int *do_profile)
    {
        int err = 0;
        try {
            if (!do_profile) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *do_profile = geopm::environment().do_profile();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }

    int geopm_env_debug_attach(int *debug_attach)
    {
        int err = 0;
        try {
            if (!debug_attach) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *debug_attach = geopm::environment().debug_attach();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }
}
