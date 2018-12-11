/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "geopm_env.h"
#include "Exception.hpp"

#include "config.h"

namespace geopm
{
    /// @brief Environment class encapsulates all functionality related to
    /// dealing with runtime environment variables.
    class Environment
    {
        public:
            Environment();
            virtual ~Environment() = default;
            void load(void);
            const char *report(void) const;
            const char *comm(void) const;
            const char *policy(void) const;
            const char *shmkey(void) const;
            const char *trace(void) const;
            const char *plugin_path(void) const;
            const char *profile(void) const;
            const char *agent(void) const;
            const char *trace_signal(int index) const;
            int max_fan_out(void) const;
            int num_trace_signal(void) const;
            int pmpi_ctl(void) const;
            int do_region_barrier(void) const;
            int do_trace(void) const;
            int do_profile() const;
            int profile_timeout(void) const;
            int debug_attach(void) const;
        private:
            bool get_env(const char *name, std::string &env_string) const;
            bool get_env(const char *name, int &value) const;
            std::string m_report;
            std::string m_comm;
            std::string m_policy;
            std::string m_agent;
            std::string m_shmkey;
            std::string m_trace;
            std::string m_plugin_path;
            std::string m_profile;
            int m_max_fan_out;
            int m_pmpi_ctl;
            bool m_do_region_barrier;
            bool m_do_trace;
            bool m_do_profile;
            int m_profile_timeout;
            int m_debug_attach;
            std::vector<std::string> m_trace_signal;
    };

    static Environment &test_environment(void)
    {
        static Environment instance;
        return instance;
    }

    static const Environment &environment(void)
    {
        return test_environment();
    }

    Environment::Environment()
    {
        load();
    }

    void Environment::load()
    {
        m_report = "";
        m_comm = "MPIComm";
        m_policy = "";
        m_agent = "monitor";
        m_shmkey = "/geopm-shm-" + std::to_string(geteuid());
        m_trace = "";
        m_plugin_path = "";
        m_profile = "";
        m_max_fan_out = 16;
        m_pmpi_ctl = GEOPM_PMPI_CTL_NONE;
        m_do_region_barrier = false;
        m_do_trace = false;
        m_do_profile = false;
        m_profile_timeout = 30;
        m_debug_attach = -1;
        m_trace_signal.clear();

        std::string tmp_str("");

        (void)get_env("GEOPM_REPORT", m_report);
        (void)get_env("GEOPM_COMM", m_comm);
        (void)get_env("GEOPM_POLICY", m_policy);
        (void)get_env("GEOPM_AGENT", m_agent);
        (void)get_env("GEOPM_SHMKEY", m_shmkey);
        if (m_shmkey[0] != '/') {
            m_shmkey = "/" + m_shmkey;
        }
        m_do_trace = get_env("GEOPM_TRACE", m_trace);
        (void)get_env("GEOPM_PLUGIN_PATH", m_plugin_path);
        m_do_region_barrier = get_env("GEOPM_REGION_BARRIER", tmp_str);
        (void)get_env("GEOPM_PROFILE_TIMEOUT", m_profile_timeout);
        if (get_env("GEOPM_PMPI_CTL", tmp_str)) {
            if (tmp_str == "process") {
                m_pmpi_ctl = GEOPM_PMPI_CTL_PROCESS;
            }
            else if (tmp_str == "pthread") {
                m_pmpi_ctl = GEOPM_PMPI_CTL_PTHREAD;
            }
            else {
                throw Exception("Environment::Environment(): " + tmp_str +
                                " is not a valid value for GEOPM_PMPI_CTL see geopm(7).",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        get_env("GEOPM_DEBUG_ATTACH", m_debug_attach);
        m_do_profile = get_env("GEOPM_PROFILE", m_profile);
        (void)get_env("GEOPM_MAX_FAN_OUT", m_max_fan_out);
        if (m_report.length() ||
            m_do_trace ||
            m_pmpi_ctl != GEOPM_PMPI_CTL_NONE) {
            m_do_profile = true;
        }
        if (m_do_profile && !m_profile.length()) {
            m_profile = program_invocation_name;
        }

        bool do_parse = get_env("GEOPM_TRACE_SIGNALS", tmp_str);
        if (do_parse) {
            std::string request;
            // split on comma
            size_t begin = 0;
            size_t end = -1;
            do {
                begin = end + 1;
                end = tmp_str.find(",", begin);
                request = tmp_str.substr(begin, end - begin);
                if (!request.empty()) {
                    m_trace_signal.push_back(request);
                }
            }
            while (end != std::string::npos);
        }

    }

    bool Environment::get_env(const char *name, std::string &env_string) const
    {
        bool result = false;
        char *check_string = getenv(name);
        if (check_string != NULL) {
            if (strlen(check_string) > NAME_MAX) {
                throw Exception("Environment::Environment(): Environment variable too long",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            env_string = check_string;
            result = true;
        }
        return result;
    }

    bool Environment::get_env(const char *name, int &value) const
    {
        bool result = false;
        std::string tmp_str("");
        char *end_ptr = NULL;

        if (get_env(name, tmp_str)) {
            value = strtol(tmp_str.c_str(), &end_ptr, 10);
            if (tmp_str.c_str() == end_ptr) {
                throw Exception("Environment::Environment(): Value could not be converted to an integer",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = true;
        }
        return result;
    }

    const char *Environment::report(void) const
    {
        return m_report.c_str();
    }

    const char *Environment::comm(void) const
    {
        return m_comm.c_str();
    }

    const char *Environment::policy(void) const
    {
        return m_policy.c_str();
    }

    const char *Environment::agent(void) const
    {
        return m_agent.c_str();
    }

    const char *Environment::shmkey(void) const
    {
        return m_shmkey.c_str();
    }

    const char *Environment::trace(void) const
    {
        return m_trace.c_str();
    }

    const char *Environment::profile(void) const
    {
        return m_profile.c_str();
    }

    const char *Environment::plugin_path(void) const
    {
        return m_plugin_path.c_str();
    }

    const char *Environment::trace_signal(int index) const
    {
        static const char *empty_string = "";
        const char *result = empty_string;
        if (index >= 0 || (size_t)index <= m_trace_signal.size()) {
            result = m_trace_signal[index].c_str();
        }
        return result;
    }

    int Environment::max_fan_out(void) const
    {
        return m_max_fan_out;
    }

    int Environment::num_trace_signal(void) const
    {
        return m_trace_signal.size();
    }

    int Environment::pmpi_ctl(void) const
    {
        return m_pmpi_ctl;
    }

    int Environment::do_region_barrier(void) const
    {
        return m_do_region_barrier;
    }

    int Environment::do_trace(void) const
    {
        return m_do_trace;
    }

    int Environment::do_profile(void) const
    {
        return m_do_profile;
    }

    int Environment::profile_timeout(void) const
    {
        return m_profile_timeout;
    }

    int Environment::debug_attach(void) const
    {
        return m_debug_attach;
    }
}

extern "C"
{
    void geopm_env_load(void)
    {
        geopm::test_environment().load();
    }

    const char *geopm_env_policy(void)
    {
        return geopm::environment().policy();
    }

    const char *geopm_env_agent(void)
    {
        return geopm::environment().agent();
    }

    const char *geopm_env_shmkey(void)
    {
        return geopm::environment().shmkey();
    }

    const char *geopm_env_trace(void)
    {
        return geopm::environment().trace();
    }

    const char *geopm_env_plugin_path(void)
    {
        return geopm::environment().plugin_path();
    }

    const char *geopm_env_report(void)
    {
        return geopm::environment().report();
    }

    const char *geopm_env_comm(void)
    {
        return geopm::environment().comm();
    }

    const char *geopm_env_profile(void)
    {
        return geopm::environment().profile();
    }
    const char *geopm_env_trace_signal(int index)
    {
        return geopm::environment().trace_signal(index);
    }

    int geopm_env_max_fan_out(void)
    {
        return geopm::environment().max_fan_out();
    }

    int geopm_env_num_trace_signal(void)
    {
        return geopm::environment().num_trace_signal();
    }

    int geopm_env_pmpi_ctl(void)
    {
        return geopm::environment().pmpi_ctl();
    }

    int geopm_env_do_region_barrier(void)
    {
        return geopm::environment().do_region_barrier();
    }

    int geopm_env_do_trace(void)
    {
        return geopm::environment().do_trace();
    }

    int geopm_env_do_profile(void)
    {
        return geopm::environment().do_profile();
    }

    int geopm_env_profile_timeout(void)
    {
        return geopm::environment().profile_timeout();
    }

    int geopm_env_debug_attach(void)
    {
        return geopm::environment().debug_attach();
    }
}
