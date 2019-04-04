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

#include "contrib/json11/json11.hpp"

#include "geopm_internal.h"
#include "Exception.hpp"
#include "Helper.hpp"

#include "config.h"

using json11::Json;

namespace geopm
{
    const std::string DEFAULT_SETTINGS_PATH = "/var/lib/geopm/environment-default.json";
    const std::string OVERRIDE_SETTINGS_PATH = "/var/lib/geopm/environment-override.json";

    static EnvironmentImp &test_environment(void)
    {
        static EnvironmentImp instance;
        return instance;
    }

    const Environment &environment(void)
    {
        return test_environment();
    }

    static void parse_environment_file(const std::string &settings_path,
                                       std::map<std::string, std::string&> &str_settings)
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
                auto str_it = str_settings.find(obj.first);
                if (str_it != str_settings.end() &&
                    obj.second.type() != Json::STRING) {
                    throw Exception("EnvironmentImp::" + std::string(__func__) +
                                    ": value for " + obj.first + " expected to be a string",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
                else {
                    str_it->second = obj.second.string_value();
                }
            }
        }
    }

    EnvironmentImp::EnvironmentImp()
        : EnvironmentImp(DEFAULT_SETTINGS_PATH, OVERRIDE_SETTINGS_PATH)
    {
    }

    EnvironmentImp::EnvironmentImp(const std::string &default_settings_path, const std::string &override_settings_path)
    {
        load(default_settings_path, override_settings_path);
    }

    void EnvironmentImp::load(void)
    {
        load(DEFAULT_SETTINGS_PATH, OVERRIDE_SETTINGS_PATH);
    }

    void EnvironmentImp::load(const std::string &default_settings_path, const std::string &override_settings_path)
    {
        m_vars.report = "";
        m_vars.comm = "MPIComm";
        m_vars.policy = "";
        m_vars.agent = "monitor";
        m_vars.shmkey = "/geopm-shm-" + std::to_string(geteuid());
        m_vars.trace = "";
        m_vars.plugin_path = "";
        m_vars.profile = "";
        m_vars.frequency_map = "";
        m_vars.max_fan_out = std::to_string(16);
        m_vars.pmpi_ctl_str = "none";
        m_vars.timeout = std::to_string(30);
        m_vars.debug_attach = std::to_string(-1);
        m_vars.trace_signals = "";
        m_vars.report_signals = "";

        m_exp_str_type.emplace("GEOPM_TRACE", m_vars.trace);
        m_exp_str_type.emplace("GEOPM_PROFILE", m_vars.profile);
        m_exp_str_type.emplace("GEOPM_REPORT", m_vars.report);
        m_exp_str_type.emplace("GEOPM_COMM", m_vars.comm);
        m_exp_str_type.emplace("GEOPM_POLICY", m_vars.policy);
        m_exp_str_type.emplace("GEOPM_AGENT", m_vars.agent);
        m_exp_str_type.emplace("GEOPM_SHMKEY", m_vars.shmkey);
        m_exp_str_type.emplace("GEOPM_FREQUENCY_MAP", m_vars.frequency_map);
        m_exp_str_type.emplace("GEOPM_TRACE_SIGNALS", m_vars.trace_signals);
        m_exp_str_type.emplace("GEOPM_REPORT_SIGNALS", m_vars.report_signals);
        m_exp_str_type.emplace("GEOPM_PLUGIN_PATH", m_vars.plugin_path);
        m_exp_str_type.emplace("GEOPM_CTL", m_vars.pmpi_ctl_str);
        m_exp_str_type.emplace("GEOPM_DEBUG_ATTACH", m_vars.debug_attach);
        m_exp_str_type.emplace("GEOPM_MAX_FAN_OUT", m_vars.max_fan_out);
        m_exp_str_type.emplace("GEOPM_TIMEOUT", m_vars.timeout);
        m_exp_str_type.emplace("GEOPM_REGION_BARRIER", m_vars.region_barrier);

        parse_environment_file(default_settings_path, m_exp_str_type);

        (void)get_env("GEOPM_CTL", m_vars.pmpi_ctl_str);
        (void)get_env("GEOPM_REPORT", m_vars.report);
        (void)get_env("GEOPM_COMM", m_vars.comm);
        (void)get_env("GEOPM_POLICY", m_vars.policy);
        (void)get_env("GEOPM_AGENT", m_vars.agent);
        (void)get_env("GEOPM_SHMKEY", m_vars.shmkey);
        (void) get_env("GEOPM_TRACE", m_vars.trace);
        (void)get_env("GEOPM_PLUGIN_PATH", m_vars.plugin_path);
        (void)get_env("GEOPM_REGION_BARRIER", m_vars.region_barrier);
        (void)get_env("GEOPM_TIMEOUT", m_vars.timeout);
        (void)get_env("GEOPM_DEBUG_ATTACH", m_vars.debug_attach);
        (void)get_env("GEOPM_PROFILE", m_vars.profile);
        (void)get_env("GEOPM_FREQUENCY_MAP", m_vars.frequency_map);
        (void)get_env("GEOPM_MAX_FAN_OUT", m_vars.max_fan_out);
        (void)get_env("GEOPM_TRACE_SIGNALS", m_vars.trace_signals);
        (void)get_env("GEOPM_REPORT_SIGNALS", m_vars.report_signals);

        parse_environment_file(override_settings_path, m_exp_str_type);
    }

    bool EnvironmentImp::get_env(const char *name, std::string &env_string) const
    {
        bool result = false;
        char *check_string = getenv(name);
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

    bool EnvironmentImp::get_env(const char *name, int &value) const
    {
        bool result = false;
        std::string tmp_str("");
        char *end_ptr = NULL;

        if (get_env(name, tmp_str)) {
            value = strtol(tmp_str.c_str(), &end_ptr, 10);
            if (tmp_str.c_str() == end_ptr) {
                throw Exception("EnvironmentImp::EnvironmentImp(): Value could not be converted to an integer",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = true;
        }
        return result;
    }

    std::string EnvironmentImp::report(void) const
    {
        return m_vars.report;
    }

    std::string EnvironmentImp::comm(void) const
    {
        return m_vars.comm;
    }

    std::string EnvironmentImp::policy(void) const
    {
        return m_vars.policy;
    }

    std::string EnvironmentImp::agent(void) const
    {
        return m_vars.agent;
    }

    std::string EnvironmentImp::shmkey(void) const
    {
        std::string ret = m_vars.shmkey;
        if (ret[0] != '/') {
            ret.insert(0, "/");
        }
        return ret;
    }

    std::string EnvironmentImp::trace(void) const
    {
        return m_vars.trace;
    }

    std::string EnvironmentImp::profile(void) const
    {
        std::string ret = m_vars.profile;
        if (!ret.length()) {
            ret = std::string(program_invocation_name);
        }
        return ret;
    }

    std::string EnvironmentImp::frequency_map(void) const
    {
        return m_vars.frequency_map;
    }

    std::string EnvironmentImp::plugin_path(void) const
    {
        return m_vars.plugin_path;
    }

    std::string EnvironmentImp::trace_signals(void) const
    {
        return m_vars.trace_signals;
    }

    std::string EnvironmentImp::report_signals(void) const
    {
        return m_vars.report_signals;
    }

    int EnvironmentImp::max_fan_out(void) const
    {
        return std::stoi(m_vars.max_fan_out);
    }

    int EnvironmentImp::pmpi_ctl(void) const
    {
        int ret = GEOPM_CTL_NONE;
        if (m_vars.pmpi_ctl_str == "process") {
            ret = GEOPM_CTL_PROCESS;
        }
        else if (m_vars.pmpi_ctl_str == "pthread") {
            ret = GEOPM_CTL_PTHREAD;
        }
        else {
            throw Exception("EnvironmentImp::EnvironmentImp(): " + m_vars.pmpi_ctl_str +
                            " is not a valid value for GEOPM_CTL see geopm(7).",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return ret;
    }

    int EnvironmentImp::do_region_barrier(void) const
    {
        return m_vars.region_barrier.length() != 0;
    }

    int EnvironmentImp::do_trace(void) const
    {
        return m_vars.trace.length() != 0;
    }

    int EnvironmentImp::do_profile(void) const
    {
        int ret = 0;
        if (m_vars.report.length() ||
            m_vars.trace.length() != 0 ||
            m_vars.pmpi_ctl_str != "none") {
            ret = true;
        }
        return ret;
    }

    int EnvironmentImp::timeout(void) const
    {
        return std::stoi(m_vars.timeout);
    }

    int EnvironmentImp::debug_attach(void) const
    {
        return std::stoi(m_vars.debug_attach);
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
        }
        return err;
    }
}
