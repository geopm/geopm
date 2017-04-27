/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <string>
#include <unistd.h>

#include "geopm_env.h"
#include "Exception.hpp"


namespace geopm
{
    /// @brief Environment class encapsulates all functionality related to
    /// dealing with runtime environment variables.
    class Environment
    {
        public:
            Environment();
            virtual ~Environment();
            const char *report(void) const;
            const char *policy(void) const;
            const char *shmkey(void) const;
            const char *trace(void) const;
            const char *plugin_path(void) const;
            const char *profile(void) const;
            int report_verbosity(void) const;
            int pmpi_ctl(void) const;
            int do_region_barrier(void) const;
            int do_trace(void) const;
            int do_ignore_affinity() const;
            int do_profile() const;
            int profile_timeout(void) const;
            int debug_attach(void) const;
        private:
            bool get_env(const char *name, std::string &env_string) const;
            bool get_env(const char *name, int &value) const;
            std::string m_report;
            std::string m_policy;
            std::string m_shmkey;
            std::string m_trace;
            std::string m_plugin_path;
            std::string m_profile;
            int m_report_verbosity;
            int m_pmpi_ctl;
            bool m_do_region_barrier;
            bool m_do_trace;
            bool m_do_ignore_affinity;
            bool m_do_profile;
            int m_profile_timeout;
            int m_debug_attach;
    };

    static const Environment &environment(void)
    {
        static const Environment instance;
        return instance;
    }

    Environment::Environment()
        : m_report("")
        , m_policy("")
        , m_shmkey("/geopm-shm")
        , m_trace("")
        , m_plugin_path("")
        , m_profile("")
        , m_report_verbosity(0)
        , m_pmpi_ctl(GEOPM_PMPI_CTL_NONE)
        , m_do_region_barrier(false)
        , m_do_trace(false)
        , m_do_ignore_affinity(false)
        , m_do_profile(false)
        , m_profile_timeout(30)
        , m_debug_attach(-1)
    {
        std::string tmp_str("");

        (void)get_env("GEOPM_REPORT", m_report);
        (void)get_env("GEOPM_POLICY", m_policy);
        (void)get_env("GEOPM_SHMKEY", m_shmkey);
        m_shmkey += "-" + std::to_string(geteuid());
        m_do_trace = get_env("GEOPM_TRACE", m_trace);
        (void)get_env("GEOPM_PLUGIN_PATH", m_plugin_path);
        if (!get_env("GEOPM_REPORT_VERBOSITY", m_report_verbosity) && m_report.size()) {
            m_report_verbosity = 1;
        }
        m_do_region_barrier = get_env("GEOPM_REGION_BARRIER", tmp_str);
        m_do_ignore_affinity = get_env("GEOPM_ERROR_AFFINITY_IGNORE", tmp_str);
        (void)get_env("GEOPM_PROFILE_TIMEOUT", m_profile_timeout);
        if (get_env("GEOPM_PMPI_CTL", tmp_str)) {
            if (tmp_str == "process") {
                m_pmpi_ctl = GEOPM_PMPI_CTL_PROCESS;
            }
            else if (tmp_str == "pthread") {
                m_pmpi_ctl = GEOPM_PMPI_CTL_PTHREAD;
            }
        }
        get_env("GEOPM_DEBUG_ATTACH", m_debug_attach);
        m_do_profile = get_env("GEOPM_PROFILE", m_profile);
        if (m_report.length() ||
            m_do_trace ||
            m_pmpi_ctl != GEOPM_PMPI_CTL_NONE) {
            m_do_profile = true;
        }
        if (m_do_profile && !m_profile.length()) {
            m_profile = program_invocation_name;
        }
    }

    Environment::~Environment()
    {

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

    const char *Environment::policy(void) const
    {
        return m_policy.c_str();
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

    int Environment::report_verbosity(void) const
    {
        return m_report_verbosity;
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

    int Environment::do_ignore_affinity(void) const
    {
        return m_do_ignore_affinity;
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
    const char *geopm_env_policy(void)
    {
        return geopm::environment().policy();
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

    const char *geopm_env_profile(void)
    {
        return geopm::environment().profile();
    }

    int geopm_env_report_verbosity(void)
    {
        return geopm::environment().report_verbosity();
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

    int geopm_env_do_ignore_affinity(void)
    {
        return geopm::environment().do_ignore_affinity();
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
