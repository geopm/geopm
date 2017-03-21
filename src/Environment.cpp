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

#include "geopm_env.h"
#include "Exception.hpp"

int geopm_getenv(const char *name, char **env_string)
{
    int err = 0;
    char *check_string = NULL;
    (*env_string) = NULL;
    check_string = getenv(name);
    if (check_string != NULL) {
        size_t strsize = (strlen(check_string) <  (NAME_MAX - 1)) ? (strlen(check_string) + 1) : NAME_MAX;
        (*env_string) = (char*)malloc(strsize);
        if ((*env_string) != NULL) {
            strncpy((*env_string), check_string, strsize);
            (*env_string)[strsize-1] = '\0';
        }
        else {
            err = errno;
        }
    }
    return err;
}

namespace geopm
{
    /// @brief Environment class encapuslaes all functionality related to
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
            int report_verbosity(void) const;
            int pmpi_ctl(void) const;
            int do_region_barrier(void) const;
            int do_trace(void) const;
            int do_ignore_affinity() const;
            int do_profile() const;
            int profile_timeout(void) const;
        private:
            std::string m_report_env;
            std::string m_policy_env;
            std::string m_shmkey_env;
            std::string m_trace_env;
            std::string m_plugin_path_env;
            int m_report_verbosity;
            int m_pmpi_ctl;
            bool m_do_region_barrier;
            bool m_do_trace;
            bool m_do_ignore_affinity;
            bool m_do_profile;
            int m_profile_timeout;
    };

    static const Environment &environment(void)
    {
        static const Environment instance;
        return instance;
    }

    Environment::Environment()
    {
        int err = 0;
        char *env_string = NULL;
        char *end_string = NULL;
        err = geopm_getenv("GEOPM_REPORT", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_report_env = env_string ? env_string : "";
        err = geopm_getenv("GEOPM_POLICY", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_policy_env = env_string ? env_string : "";
        err = geopm_getenv("GEOPM_SHMKEY", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_shmkey_env = env_string ? env_string : "/geopm-shm";
        err = geopm_getenv("GEOPM_TRACE", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_trace_env = env_string ? env_string : "";
        err = geopm_getenv("GEOPM_PLUGIN_PATH", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_plugin_path_env = env_string ? env_string : "";
        err = geopm_getenv("GEOPM_REPORT_VERBOSITY", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        if (env_string != NULL) {
            m_report_verbosity = strtol(env_string, &end_string, 10);
            if (env_string == end_string) {
                throw Exception("Environment::Environment(): Read invalid value from environment", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        else {
            m_report_verbosity = m_report_env.size() ? 1 : 0;
        }
        err = geopm_getenv("GEOPM_REGION_BARRIER", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_do_region_barrier = env_string != NULL;
        err = geopm_getenv("GEOPM_TRACE", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_do_trace = env_string != NULL;
        err = geopm_getenv("GEOPM_ERROR_AFFINITY_IGNORE", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_do_ignore_affinity = env_string != NULL;
        err = geopm_getenv("GEOPM_PROFILE", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_do_profile = m_report_env.length() ||
                     m_trace_env.length() ||
                     env_string != NULL;
        err = geopm_getenv("GEOPM_PROFILE_TIMEOUT", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        m_profile_timeout = 30;
        if (env_string != NULL) {
            m_profile_timeout = strtol(env_string, &end_string, 10);
            if (env_string == end_string) {
                throw Exception("Environment::Environment(): Read invalid value from environment", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        err = geopm_getenv("GEOPM_PMPI_CTL", &env_string);
        if (err) {
            throw Exception("Environment::Environment(): Could read from environment", err, __FILE__, __LINE__);
        }
        if (env_string && !strncmp(env_string, "process", strlen("process") + 1))  {
            m_pmpi_ctl = GEOPM_PMPI_CTL_PROCESS;
            m_do_profile = true;
        }
        else if (env_string && !strncmp(env_string, "pthread", strlen("pthread") + 1))  {
            m_pmpi_ctl = GEOPM_PMPI_CTL_PTHREAD;
            m_do_profile = true;
        }
        else {
            m_pmpi_ctl = GEOPM_PMPI_CTL_NONE;
        }
    }

    Environment::~Environment()
    {

    }

    const char *Environment::report(void) const
    {
        return m_report_env.c_str();
    }

    const char *Environment::policy(void) const
    {
        return m_policy_env.c_str();
    }

    const char *Environment::shmkey(void) const
    {
        return m_shmkey_env.c_str();
    }

    const char *Environment::trace(void) const
    {
        return m_trace_env.c_str();
    }

    const char *Environment::plugin_path(void) const
    {
        return m_plugin_path_env.c_str();
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

    int Environment::do_ignore_affinity() const
    {
        return m_do_ignore_affinity;
    }

    int Environment::do_profile() const
    {
        return m_do_profile;
    }

    int Environment::profile_timeout() const
    {
        return m_profile_timeout;
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
}
